#include "pch.h"
#include "log.h"
#include "str.h"
#include "font.h"
#include "profile.h"
#include "asset.h"
#include "input_stack.h"
#include "reflection.h"

#include "stb_sprintf.h"
#include "stb_image.h"

#include "os/system.h"
#include "os/thread.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"
#include "os/file.h"

#include "math/math_basic.h"

#include "render/render.h"
#include "render/ui.h"
#include "render/r_table.h"
#include "render/r_command.h"
#include "render/r_target.h"
#include "render/r_pass.h"
#include "render/r_stats.h"
#include "render/r_storage.h"
#include "render/r_geo.h"
#include "render/r_viewport.h"
#include "render/r_shader.h"
#include "render/r_texture.h"
#include "render/r_material.h"
#include "render/r_uniform.h"
#include "render/r_mesh.h"
#include "render/r_flip_book.h"

#include "audio/audio.h"
#include "audio/au_table.h"
#include "audio/au_sound.h"

#include "game/world.h"
#include "game/game.h"

#include "editor/editor.h"
#include "editor/hot_reload.h"
#include "editor/debug_console.h"
#include "editor/telemetry.h"

void on_window_event(const Window &window, const Window_Event &event) {
    switch (event.type) {
	case WINDOW_EVENT_RESIZE: {
        on_window_resize(window.width, window.height);
        break;
	}
    case WINDOW_EVENT_KEYBOARD:
	case WINDOW_EVENT_TEXT_INPUT:
	case WINDOW_EVENT_MOUSE: {
        const auto *layer = get_current_input_layer();
        if (layer) {
            layer->on_input(event);
        }
        break;
    }
	case WINDOW_EVENT_QUIT: {
		log("WINDOW_EVENT_QUIT");
        break;
	}
    default: {
        error("Unhandled window event %d", event.type);
        break;
    }
    }
}

s32 main() {    
    START_SCOPE_TIMER(startup);

    os_init();

    if (!reserve(M_global, MB(4))) return 1;    
    if (!reserve(M_frame,  MB(2))) return 1;
    
    defer { release(M_global); };
    defer { release(M_frame); };

    tm_init();
    sid_init();

    sid_texture_player_idle[SOUTH] = SID_TEXTURE_PLAYER_IDLE_SOUTH;
    sid_texture_player_idle[EAST]  = SID_TEXTURE_PLAYER_IDLE_EAST;
    sid_texture_player_idle[WEST]  = SID_TEXTURE_PLAYER_IDLE_WEST;
    sid_texture_player_idle[NORTH] = SID_TEXTURE_PLAYER_IDLE_NORTH;
    
	init_input_table();

    {
        Input_layer_game.type = INPUT_LAYER_GAME;
        Input_layer_game.on_input = on_input_game;

        Input_layer_editor.type = INPUT_LAYER_EDITOR;
        Input_layer_editor.on_input = on_input_editor;

        Input_layer_debug_console.type = INPUT_LAYER_DEBUG_CONSOLE;
        Input_layer_debug_console.on_input = on_input_debug_console;

        Input_layer_tm.type = INPUT_LAYER_TM;
        Input_layer_tm.on_input = tm_on_input;

        Input_layer_mprof.type = INPUT_LAYER_MPROF;
        Input_layer_mprof.on_input = mprof_on_input;

        push_input_layer(Input_layer_editor);
    }
    
	if (os_create_window(1920, 1080, GAME_NAME, 0, 0, Main_window) == false) {
		error("Failed to create window");
		return 1;
	}

	os_register_window_callback(Main_window, on_window_event);
    defer { os_destroy_window(Main_window); };

    if (!r_init_context(Main_window)) {
        error("Failed to initialize render context");
        return 1;
    }

    {
        R_Pass pass;
        pass.scissor.test = R_ENABLE;
        pass.cull.test = R_ENABLE;
        pass.blend.test = R_ENABLE;
        pass.depth.test = R_ENABLE;
        pass.stencil.test = R_ENABLE;

        r_submit(pass);
    }
    
    r_detect_capabilities();

    {
        const u32 map_bits = R_MAP_WRITE_BIT | R_MAP_PERSISTENT_BIT | R_MAP_COHERENT_BIT;
        const u32 storage_bits = R_DYNAMIC_STORAGE_BIT | map_bits;
        
        static R_Storage vstorage;
        r_create_storage(MB(32), storage_bits, vstorage);
        R_vertex_map_range = r_map(vstorage, 0, MB(32), map_bits);

        static R_Storage istorage;
        r_create_storage(MB(2), storage_bits, istorage);
        R_index_map_range = r_map(istorage, 0, MB(2), map_bits);

#if DEVELOPER
        R_eid_alloc_range = r_alloc(R_vertex_map_range, MB(1));
#endif
    }

    r_init_global_uniforms();
    r_create_table(R_table);
    defer { r_destroy_table(R_table); };
    
    au_init_context();
    au_create_table(Au_table);
    defer { au_destroy_table(Au_table); };
    
    os_lock_window_cursor(Main_window, true);
    os_set_window_vsync(Main_window, true);

    stbi_set_flip_vertically_on_load(true);

    init_asset_source_table();
    init_asset_table();

#if DEVELOPER
    save_asset_pack(GAME_PAK_PATH);
#endif
    load_asset_pack(GAME_PAK_PATH);
        
    R_viewport.aspect_type = VIEWPORT_4X3;
    R_viewport.resolution_scale = 1.0f;
    R_viewport.quantize_color_count = 256;

    const u16 cformats[] = { R_RGB_8, R_RED_32 };
    R_viewport.render_target = r_create_render_target(Main_window.width, Main_window.height,
                                                      COUNT(cformats), cformats,
                                                      R_DEPTH_24_STENCIL_8);
    
#if 0
    R_viewport.pixel_size                  = 1.0f;
    R_viewport.curve_distortion_factor     = 0.25f;
    R_viewport.chromatic_aberration_offset = 0.002f;
    R_viewport.quantize_color_count        = 16;
    R_viewport.noise_blend_factor          = 0.1f;
    R_viewport.scanline_count              = 64;
    R_viewport.scanline_intensity          = 0.95f;
#endif
    
    r_resize_viewport(R_viewport, Main_window.width, Main_window.height);

    ui_init();
    r_geo_init();

    init_debug_console();
    
    // @Cleanup: just make it better.
    extern void r_init_frame_buffer_draw();
    r_init_frame_buffer_draw();
    
	Hot_Reload_List hot_reload_list = {};
    // @Note: shader includes does not count as shader hot reload.
	register_hot_reload_directory(hot_reload_list, DIR_SHADERS);
	register_hot_reload_directory(hot_reload_list, DIR_TEXTURES);
	register_hot_reload_directory(hot_reload_list, DIR_MATERIALS);
	register_hot_reload_directory(hot_reload_list, DIR_MESHES);
	register_hot_reload_directory(hot_reload_list, DIR_FLIP_BOOKS);

    const Thread hot_reload_thread = start_hot_reload_thread(hot_reload_list);
    defer { os_terminate_thread(hot_reload_thread); };
    
	create_world(World);

#if 1
    init_default_level(World);
    save_level(World);
#else
    const String main_level_path = PATH_LEVEL("main.lvl");
    load_level(World, main_level_path);
#endif
    
    log("Startup took %.2fms", CHECK_SCOPE_TIMER_MS(startup));

    u64 begin_counter = os_perf_counter();
	while (os_window_alive(Main_window)) {
        // @Note: event queue is NOT cleared after this call as some parts of the code
        // want to know which events were polled. The queue is cleared during buffer swap.
		os_poll_window_events(Main_window);

        R_viewport.mouse_pos = vec2(Clamp((f32)input_table.mouse_x - R_viewport.x, 0.0f, R_viewport.width),
                                  Clamp((f32)input_table.mouse_y - R_viewport.y, 0.0f, R_viewport.height));
        
        check_hot_reload(hot_reload_list);

        tick_game(Delta_time);
        tick_editor(Delta_time);
        
        draw_world(World);

        //editor_report("%s", to_string(World.player.velocity));
        ui_world_line(vec3_zero, vec3_zero + vec3(0.5f, 0.0f, 0.0f), rgba_red);
        // ui_world_line(World.player.location,
        //               World.player.location + normalize(World.player.velocity) * 1.0f,
        //               rgba_red);

#if DEVELOPER
        r_geo_debug();
        draw_dev_stats();
        draw_debug_console();
        mprof_draw();
#endif
        
#if 0
        static f32 pixel_size_time = -1.0f;
        if (pixel_size_time > 180.0f) pixel_size_time = 0.0f;
        R_viewport.frame_buffer.pixel_size = (Sin(pixel_size_time) + 1.0f) * R_viewport.frame_buffer.width * 0.05f;
        pixel_size_time += delta_time * 4.0f;
#endif

        r_world_flush();
        r_geo_flush();
        r_viewport_flush();
        ui_flush(); // ui is drawn directly to screen
        
		os_swap_window_buffers(Main_window);
        r_update_stats();

#if DEVELOPER
        // Queue telemetry profiler draw. We are doing it here, so
        // telemetry zones will be listed in order of push by default.
        tm_draw();
#endif
        
        clear(M_frame);
        
		const u64 end_counter = os_perf_counter();
		Delta_time = (f32)(end_counter - begin_counter) / os_perf_hz();
		begin_counter = end_counter;
        
#if DEVELOPER
        // If dt is too large, we could have resumed from a breakpoint.
        if (Delta_time > 1.0f) {
            Delta_time = 0.16f;
        }
#endif
	}
    
	return 0;
}
