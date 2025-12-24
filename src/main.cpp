#include "pch.h"
#include "virtual_arena.h"

// @Todo: define stb related macros that allow to override usage of std.

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#undef STB_SPRINTF_IMPLEMENTATION

#define STBI_ASSERT(x)     Assert(x)
#define STBI_MALLOC(n)     alloc  (n,    { .proc = __default_allocator_proc })
#define STBI_REALLOC(p, n) resize (p, n, { .proc = __default_allocator_proc })
#define STBI_FREE(p)       release(p,    { .proc = __default_allocator_proc })

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

Virtual_Arena virtual_arena;

s32 main() {    
    virtual_arena.reserve_size = Megabytes(2);
    context.allocator = { .proc = virtual_arena_allocator_proc, .data = &virtual_arena };
    
    set_process_cwd(get_process_directory());
    log("Current working directory %S", get_process_directory());

    stbi_set_flip_vertically_on_load(true);

    auto window = main_window = new_window(1920, 1080, GAME_NAME);
	if (!window) {
		log(LOG_MINIMAL, "Failed to create window");
		return 1;
	}

    defer { destroy(window); };

    if (!init_render_context(window)) {
        log(LOG_MINIMAL, "Failed to initialize render context");
        return 1;
    }
    
    lock_cursor(window, true);
    set_vsync  (window, true);

    if (!init_audio_player()) return 1;

    init_asset_storages();

    init_profiler();
    init_render_frame();
    init_shader_platform();

    auto &viewport = screen_viewport;
    viewport.aspect_type = VIEWPORT_4X3;
    init(viewport, window->width, window->height);

    init_console();
    init_line_geometry();

    load_game_assets();
    init_ui(); // @Todo: check why loading ui before game assets causes a crash
    
    init_hot_reload();
    // @Note: shader includes does not count in shader hot reload.
	add_hot_reload_directory(PATH_SHADER(""));
    add_hot_reload_directory(PATH_TEXTURE(""));
    add_hot_reload_directory(PATH_FLIP_BOOK(""));
    add_hot_reload_directory(PATH_MATERIAL(""));
    add_hot_reload_directory(PATH_MESH(""));

    const auto hot_reload_thread = start_hot_reload_thread();
    defer { terminate_thread(hot_reload_thread); };

    auto set = init_editor_level_set();
    switch_campaign(set);
    init_level_editor_hub();

    init_input();
    handle_window_events(); // handle events that were sent during init phase

    game_logger_data.messages.allocator = __temporary_allocator;
    context.logger = { .proc = game_logger_proc, .data = &game_logger_data };

    log("Startup time %.2fs", (get_perf_counter() - __preload_counter) / (f32)get_perf_hz());
    log("sizeof(Entity) %d", sizeof(Entity));

    {
        auto eid = get_player(get_entity_manager())->eid;
        log("player eid %u (%u %u)", eid, get_eid_index(eid), get_eid_generation(eid));
    }
    
    main_loop();
    
	return 0;
}

static void main_loop() {
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
    
    if (program_mode == MODE_GAME) {
        simulate_game();
    }
    
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

inline constexpr auto KEY_CLOSE_WINDOW        = KEY_ESCAPE;
inline constexpr auto KEY_SWITCH_PROGRAM_MODE = KEY_F11;

static void handle_window_events() {
    auto window = get_window();
    auto input  = get_input_table();
    auto layer  = get_current_input_layer();
    
    // Populate input table with event data and handle non-input events.
    For (window->events) {
        switch (it.type) {
        case WINDOW_EVENT_FOCUSED: {
            break;
        }
        case WINDOW_EVENT_LOST_FOCUS: {
            clear_bit(&input->keys, KEY_ALT);
            break;
        }
        case WINDOW_EVENT_RESIZE: {
            on_window_resize(window->width, window->height);
            break;
        }
        case WINDOW_EVENT_KEYBOARD: {
            const auto key     = it.key_code;
            const auto press   = it.input_bits & WINDOW_EVENT_PRESS_BIT;
            const auto release = it.input_bits & WINDOW_EVENT_RELEASE_BIT;

            if      (press)   { set_bit  (&input->keys, key); }
            else if (release) { clear_bit(&input->keys, key); }
            
            break;
        }
        case WINDOW_EVENT_MOUSE_MOVE: {
            input->mouse_x = it.x;
            input->mouse_y = it.y;
            break;
        }
        case WINDOW_EVENT_QUIT: {
            should_quit_game = true;
            return;
        }
        }
    }

    // Update key states.
    {
        const u256 changes = input->keys ^ input->keys_last;
        input->keys_down = changes &  input->keys;
        input->keys_up   = changes & ~input->keys;
        copy(&input->keys_last, &input->keys, sizeof(input->keys));
    }

    // Update mouse position.
    {
        input->mouse_offset_x = input->mouse_x - input->mouse_last_x;
        input->mouse_offset_y = input->mouse_y - input->mouse_last_y;

        if (window->cursor_locked) {
            auto x = input->mouse_last_x = window->width  / 2;
            auto y = input->mouse_last_y = window->height / 2;

            if (input->mouse_x != x || input->mouse_y != y) {
                set_cursor(window, x, y);
            }
        } else {
            input->mouse_last_x = input->mouse_x;
            input->mouse_last_y = input->mouse_y;
        }
    }

    // Update mouse position in screen viewport.
    {
        auto &v = screen_viewport;
        // @Cleanup: this is actually valid only for current 4x3 viewport.
        v.mouse_pos = Vector2(Clamp((f32)input->mouse_x - v.x, 0.0f, v.width),
                              Clamp((f32)input->mouse_y - v.y, 0.0f, v.height));
    }
    
    // Handle and propagate input events.
    For (window->events) {
        switch (it.type) {
        case WINDOW_EVENT_KEYBOARD:
        case WINDOW_EVENT_TEXT_INPUT:
        case WINDOW_EVENT_MOUSE_CLICK:
        case WINDOW_EVENT_MOUSE_WHEEL: {
            if (it.type == WINDOW_EVENT_KEYBOARD) {
                const auto key   = it.key_code;
                const auto press = it.input_bits & WINDOW_EVENT_PRESS_BIT;

                // Handle general inputs.
                if (press && key == KEY_CLOSE_WINDOW) {
                    close(window);
                    break;
                } else if (press && key == KEY_SWITCH_PROGRAM_MODE) {
                    program_mode = (Program_Mode)((program_mode + 1) % PROGRAM_MODE_COUNT);

                    // @Cleanup @Hack: replace game/editor input layers.
                    for (u32 i = 0; i < input_stack.layer_count; ++i) {
                        auto &layer = input_stack.layers[i];

                        if (layer.type == INPUT_LAYER_EDITOR) {
                            if (layer.on_pop) layer.on_pop();
                            layer = input_layers[INPUT_LAYER_GAME];
                            if (layer.on_push) layer.on_push();
                            break;
                        }

                        if (layer.type == INPUT_LAYER_GAME) {
                            if (layer.on_pop) layer.on_pop();
                            layer = input_layers[INPUT_LAYER_EDITOR];
                            if (layer.on_push) layer.on_push();
                            break;
                        }
                    }
                }
            }

            layer->on_input(&it);
            break;
        }
        }
    }
}
