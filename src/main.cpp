#include "pch.h"
#include "virtual_arena.h"
#include "program_layer.h"

// @Todo: define stb related macros that allow to override usage of std.

#define STBI_ASSERT(x)     Assert(x)
#define STBI_MALLOC(n)     alloc  (n,    __default_allocator)
#define STBI_REALLOC(p, n) resize (p, n, __default_allocator)
#define STBI_FREE(p)       release(p,    __default_allocator)

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION

// @Todo: get rid of this.
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#undef TINYOBJLOADER_IMPLEMENTATION

#include "basic.cpp"
#include "camera.cpp"
#include "collision.cpp"
#include "font.cpp"
#include "al.cpp"
#include "audio.cpp"
#include "editor.cpp"
#include "game.cpp"
#include "math.cpp"
#include "os.cpp"
#include "render.cpp"
#include "ui.cpp"
#include "asset.cpp"

#ifdef WIN32
#include "win32.cpp"

#ifdef OPEN_GL
#include "gl.cpp"
#include "glad.cpp"
#include "win32_gl.cpp"
#else
#error "Unsupported rendering backend"
#endif

#else
#error "Unsupported platform"
#endif

static void main_loop();
static void do_one_frame();
static void update_time();
static void handle_window_events();

Virtual_Arena virtual_arena          = { .reserve_size = Megabytes(2)  };
Virtual_Arena overflow_virtual_arena = { .reserve_size = Megabytes(64) };

s32 main() {
    stbi_set_flip_vertically_on_load(true);

    game_logger_data.allocator          = __temporary_allocator;
    game_logger_data.messages.allocator = __temporary_allocator;

    context.logger    = { game_logger_proc, &game_logger_data };
    context.allocator = { virtual_arena_allocator_proc, &virtual_arena };
    context.temporary_storage->overflow_allocator = { virtual_arena_allocator_proc, &overflow_virtual_arena };
    
    set_process_cwd(get_process_directory());

    const auto cwd = get_process_directory();
    log("Current working directory %S", cwd);

    init_program_layers();
    
    auto window = main_window = new_window(1920, 1080, GAME_NAME);
	if (!window) {
		log(LOG_ERROR, "Failed to create window");
		return 1;
	}

    defer { destroy(window); };

    if (!init_render_context(window)) {
        log(LOG_ERROR, "Failed to initialize render context");
        return 1;
    }
    
    lock_cursor(window, true);
    set_vsync  (window, true);

    if (!init_audio_player()) return 1;
    
    init_asset_storages();
    init_profiler();
    init_render_frame();
    init_shader_platform();
    init_line_geometry();

    auto &viewport = screen_viewport;
    viewport.aspect_type = VIEWPORT_4X3;
    init(viewport, window->width, window->height);

    load_game_assets();

    init_ui();
        
    init_hot_reload();
    // @Note: shader includes does not count in shader hot reload.
	add_hot_reload_directory(DIR_SHADERS);
    add_hot_reload_directory(DIR_TEXTURES);
    add_hot_reload_directory(DIR_FLIP_BOOKS);
    add_hot_reload_directory(DIR_MATERIALS);
    add_hot_reload_directory(DIR_MESHES);

    const auto hot_reload_thread = start_hot_reload_thread();
    defer { terminate_thread(hot_reload_thread); };
    
    game_state.polygon_mode = GPU_POLYGON_FILL;

#if DEVELOPER
    init_level_editor_hub();
    push_program_layer(&program_layer_editor);
#else
    push_program_layer(&program_layer_game);
#endif

    handle_window_events(); // handle events that were sent during init phase

    log("Startup time %.2fs", (get_perf_counter() - __preload_counter) / (f32)get_perf_hz());
    log("sizeof(Entity) %d", sizeof(Entity));
    log("sizeof(Gpu_Command) %d", sizeof(Gpu_Command));
    
    auto eid = get_player(get_entity_manager())->eid;
    log("player eid %u (%u %u)", eid, get_eid_index(eid), get_eid_generation(eid));

    flush_game_logger();
    main_loop();
        
	return 0;
}

static void main_loop() {
    in_main_loop = true;
    defer { in_main_loop = false; };
    
    while (1) {
        if (should_quit_game) return;
        do_one_frame();
    }
}

static void do_one_frame() {
    frame_index += 1;

    highest_water_mark = Max(highest_water_mark, context.temporary_storage->high_water_mark);
    reset_temporary_storage();

    update_time();

    poll_events(get_window()); // accumulate latest window events
    handle_window_events();    // and handle all of them at once

    update_hot_reload();
    simulate_game();
    update_editor();
    render_one_frame();
    update_audio();

    // @Cleanup: move in game simulate and editor update?
    post_frame_cleanup(get_entity_manager()); 
    
    update_profiler();
    flush_game_logger();
}

static void update_time() {
    auto &start = time_info.frame_start_counter;
    auto &end   = time_info.frame_end_counter;
    auto &dt    = time_info.delta_time;
    
    end   = get_perf_counter();
    dt    = (f32)(end - start) / get_perf_hz();
    start = end;

#if DEVELOPER
    // If dt is too large, we could have resumed from a breakpoint.
    if (dt > 1.0f) dt = 0.16f;
#endif
}

static void handle_window_events() {
    process_program_layer_stack();
    
    auto window = get_window();
    auto input  = get_input_table();
    
    input->cursor_offset_x = 0;
    input->cursor_offset_y = 0;

    auto base_layer = &program_layer_base;
    For (window->events) send_event(base_layer, &it);

    const u256 changes = input->keys ^ input->keys_last;
    input->keys_down = changes &  input->keys;
    input->keys_up   = changes & ~input->keys;
    copy(&input->keys_last, &input->keys, sizeof(input->keys));

    // @Cleanup: this is actually valid only for current 4x3 viewport.
    auto &viewport = screen_viewport;
    const auto cursor_x = Clamp((f32)input->cursor_x - viewport.x, 0.0f, viewport.width);
    const auto cursor_y = Clamp((f32)input->cursor_y - viewport.y, 0.0f, viewport.height);
    viewport.cursor_pos = Vector2(cursor_x, cursor_y);
        
    auto current_layer = get_program_layer();
    For (window->events) send_event(current_layer, &it); 
}
