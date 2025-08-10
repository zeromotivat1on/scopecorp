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

        Input_layer_runtime_profiler.type = INPUT_LAYER_RUNTIME_PROFILER;
        Input_layer_runtime_profiler.on_input = tm_on_input;

        Input_layer_memory_profiler.type = INPUT_LAYER_MEMORY_PROFILER;
        Input_layer_memory_profiler.on_input = on_input_memory_profiler;

        push_input_layer(Input_layer_editor);
    }
    
	if (os_create_window(1920, 1080, GAME_NAME, 0, 0, Main_window) == false) {
		error("Failed to create window");
		return 1;
	}

    defer { os_destroy_window(Main_window); };
    
	os_register_window_callback(Main_window, on_window_event);

    if (r_init_context(Main_window) == false) {
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

    r_create_table(R_table);
    defer { r_destroy_table(R_table); };
    
    {
        const u32 map_bits = R_MAP_WRITE_BIT | R_MAP_PERSISTENT_BIT | R_MAP_COHERENT_BIT;
        const u32 storage_bits = R_DYNAMIC_STORAGE_BIT | map_bits;
        
        static R_Storage vstorage;
        r_create_storage(MB(31), storage_bits, vstorage);
        R_vertex_map_range = r_map(vstorage, 0, MB(30), map_bits);

        static R_Storage istorage;
        r_create_storage(MB(2), storage_bits, istorage);
        R_index_map_range = r_map(istorage, 0, MB(2), map_bits);

#if DEVELOPER
        R_eid_alloc_range = r_alloc(R_vertex_map_range, MB(1));
#endif
    }
    
    r_init_global_uniforms();

    // @Cleanup: use own arena?
    uniform_value_cache.data = push(M_global, MAX_UNIFORM_VALUE_CACHE_SIZE);
    uniform_value_cache.size = 0;
    uniform_value_cache.capacity = MAX_UNIFORM_VALUE_CACHE_SIZE;

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
    R_viewport.quantize_color_count = 32;

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

    constexpr u32 MAX_COMMANDS = 512;
    r_create_command_list(MAX_COMMANDS, R_command_list);
    
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
    
	delta_time = 0.0f;
	s64 begin_counter = os_perf_counter();

    log("Startup took %.2fms", CHECK_SCOPE_TIMER_MS(startup));
    
	while (os_window_alive(Main_window)) {
        // @Note: event queue is NOT cleared after this call as some parts of the code
        // want to know which events were polled. The queue is cleared during buffer swap.
		os_poll_window_events(Main_window);

        R_viewport.mouse_pos = vec2(Clamp((f32)input_table.mouse_x - R_viewport.x, 0.0f, R_viewport.width),
                                  Clamp((f32)input_table.mouse_y - R_viewport.y, 0.0f, R_viewport.height));
        
        check_hot_reload(hot_reload_list);

        tick_game(delta_time);
        tick_editor(delta_time);
        
        draw_world(World);
        
        //ui_world_line(World.player.location,
        //            World.player.location + normalize(World.player.velocity) * 1.0f,
        //            rgba_red);

#if DEVELOPER
        r_geo_debug();
        draw_dev_stats();
        draw_debug_console();
        draw_memory_profiler();
#endif
        
#if 0
        static f32 pixel_size_time = -1.0f;
        if (pixel_size_time > 180.0f) pixel_size_time = 0.0f;
        R_viewport.frame_buffer.pixel_size = (Sin(pixel_size_time) + 1.0f) * R_viewport.frame_buffer.width * 0.05f;
        pixel_size_time += delta_time * 4.0f;
#endif

        {
            const auto &rt = R_table.targets[R_viewport.render_target];
            
            R_Pass pass_game;
            pass_game.polygon = game_state.polygon_mode;
            pass_game.viewport.x = 0;
            pass_game.viewport.y = 0;
            pass_game.viewport.w = rt.width;
            pass_game.viewport.h = rt.height;
            pass_game.scissor.x = 0;
            pass_game.scissor.y = 0;
            pass_game.scissor.w = rt.width;
            pass_game.scissor.h = rt.height;
            // @Note: we disable cull face test for now as some models use other winding or
            // partially invisibile due to their parts have other winding.
            pass_game.cull.test    = R_DISABLE;
            pass_game.cull.face    = R_BACK;
            pass_game.cull.winding = R_CCW;
            pass_game.blend.src = R_SRC_ALPHA;
            pass_game.blend.dst = R_ONE_MINUS_SRC_ALPHA;
            pass_game.depth.mask = R_ENABLE;
            pass_game.depth.func = R_LESS;
            pass_game.stencil.mask = 0x00;
            pass_game.stencil.func.type       = R_ALWAYS;
            pass_game.stencil.func.comparator = 1;
            pass_game.stencil.func.mask       = 0xFF;
            pass_game.stencil.op.stencil_failed = R_KEEP;
            pass_game.stencil.op.depth_failed   = R_KEEP;
            pass_game.stencil.op.passed         = R_REPLACE;
            pass_game.clear.color = rgba_white;
            pass_game.clear.bits  = R_COLOR_BUFFER_BIT | R_DEPTH_BUFFER_BIT | R_STENCIL_BUFFER_BIT;
            
            r_submit(rt);
            r_submit(pass_game);

            r_submit(R_command_list);
            r_geo_flush();
        }
        
        {
            R_Pass pass_viewport;
            pass_viewport.polygon = R_FILL;
            pass_viewport.viewport.x = R_viewport.x;
            pass_viewport.viewport.y = R_viewport.y;
            pass_viewport.viewport.w = R_viewport.width;
            pass_viewport.viewport.h = R_viewport.height;
            pass_viewport.scissor.x = R_viewport.x;
            pass_viewport.scissor.y = R_viewport.y;
            pass_viewport.scissor.w = R_viewport.width;
            pass_viewport.scissor.h = R_viewport.height;
            pass_viewport.cull.face    = R_BACK;
            pass_viewport.cull.winding = R_CCW;
            pass_viewport.clear.color = rgba_black;
            pass_viewport.clear.bits  = R_COLOR_BUFFER_BIT | R_DEPTH_BUFFER_BIT | R_STENCIL_BUFFER_BIT;

            // Reset default render target (window screen).
            // @Cleanup: make it more clear.
            r_submit(R_Target {});
            r_submit(pass_viewport);

            // @Cleanup?
            // We cleared viewport buffers, so now we can disable depth test to
            // render quad on screen.
            R_Pass pass_depth;
            pass_depth.depth.mask = R_DISABLE;
            r_submit(pass_depth);
            
            const auto &mta = *find_asset(SID_MATERIAL_FRAME_BUFFER);
            
            r_set_material_uniform(mta.index, SID("u_pixel_size"),
                                   0, _sizeref(R_viewport.pixel_size));
            r_set_material_uniform(mta.index, SID("u_curve_distortion_factor"),
                                   0, _sizeref(R_viewport.curve_distortion_factor));
            r_set_material_uniform(mta.index, SID("u_chromatic_aberration_offset"),
                                   0, _sizeref(R_viewport.chromatic_aberration_offset));
            r_set_material_uniform(mta.index, SID("u_quantize_color_count"),
                                   0, _sizeref(R_viewport.quantize_color_count));
            r_set_material_uniform(mta.index, SID("u_noise_blend_factor"),
                                   0, _sizeref(R_viewport.noise_blend_factor));
            r_set_material_uniform(mta.index, SID("u_scanline_count"),
                                   0, _sizeref(R_viewport.scanline_count));
            r_set_material_uniform(mta.index, SID("u_scanline_intensity"),
                                   0, _sizeref(R_viewport.scanline_intensity));

            const auto &mt = R_table.materials[mta.index];
            const auto &rt = R_table.targets[R_viewport.render_target];

            extern u16 fb_vd;
            extern u32 fb_first_index;
            extern u32 fb_index_count;

            R_Command cmd;
            cmd.bits = R_CMD_INDEXED_BIT;
            cmd.mode = R_TRIANGLES;
            cmd.shader = mt.shader;
            cmd.texture = rt.color_attachments[0].texture;
            cmd.uniform_count = mt.uniform_count;
            cmd.uniforms = mt.uniforms;
            cmd.vertex_desc = fb_vd;
            cmd.first = fb_first_index;
            cmd.count = fb_index_count;
            
            r_add(R_command_list, cmd);

            r_submit(R_command_list);
        }

        {
            R_Pass pass_ui;
            pass_ui.polygon = R_FILL;
            pass_ui.viewport.x = R_viewport.x;
            pass_ui.viewport.y = R_viewport.y;
            pass_ui.viewport.w = R_viewport.width;
            pass_ui.viewport.h = R_viewport.height;
            pass_ui.scissor.x = R_viewport.x;
            pass_ui.scissor.y = R_viewport.y;
            pass_ui.scissor.w = R_viewport.width;
            pass_ui.scissor.h = R_viewport.height;
            pass_ui.cull.face    = R_BACK;
            pass_ui.cull.winding = R_CCW;
            pass_ui.blend.src = R_SRC_ALPHA;
            pass_ui.blend.dst = R_ONE_MINUS_SRC_ALPHA;
            pass_ui.depth.mask = R_DISABLE;

            r_submit(pass_ui);
            
            ui_flush(); // ui is drawn directly to screen
        }
            
		os_swap_window_buffers(Main_window);
        update_render_stats();

#if DEVELOPER
        // Queue telemetry profiler draw. We are doing it here, so
        // telemetry zones will be listed in order of push by default.
        tm_draw();
#endif
        
        clear(M_frame);
        
		const s64 end_counter = os_perf_counter();
		delta_time = (end_counter - begin_counter) / (f32)os_perf_hz();
		begin_counter = end_counter;
        
#if DEVELOPER
        // If dt is too large, we could have resumed from a breakpoint.
        if (delta_time > 1.0f) {
            delta_time = 0.16f;
        }
#endif
	}
    
	return 0;
}
