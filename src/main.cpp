#include "pch.h"
#include "log.h"
#include "str.h"
#include "sid.h"
#include "font.h"
#include "flip_book.h"
#include "profile.h"
#include "asset.h"
#include "input_stack.h"

#include "stb_sprintf.h"
#include "stb_image.h"

#include "os/thread.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"
#include "os/file.h"

#include "math/math_core.h"

#include "render/render_init.h"
#include "render/render_stats.h"
#include "render/render_command.h"
#include "render/buffer_storage.h"
#include "render/geometry.h"
#include "render/viewport.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/material.h"
#include "render/uniform.h"
#include "render/mesh.h"
#include "render/ui.h"

#include "audio/sound.h"

#include "game/world.h"
#include "game/game.h"

#include "editor/editor.h"
#include "editor/hot_reload.h"
#include "editor/debug_console.h"

void on_window_event(Window *window, Window_Event *event) {
    switch (event->type) {
	case WINDOW_EVENT_RESIZE: {
        on_window_resize(window->width, window->height);
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
        error("Unhandled window event %d", event->type);
        break;
    }
    }
}

s32 main() {
    START_SCOPE_TIMER(startup);
    
    if (!alloc_init()) {
        error("Failed to initialize allocation");
        return 1;
    }

    sid_init();
	init_input_table();

    {
        input_layer_game.type = INPUT_LAYER_GAME;
        input_layer_game.on_input = on_input_game;

        input_layer_editor.type = INPUT_LAYER_EDITOR;
        input_layer_editor.on_input = on_input_editor;

        input_layer_debug_console.type = INPUT_LAYER_DEBUG_CONSOLE;
        input_layer_debug_console.on_input = on_input_debug_console;

        input_layer_runtime_profiler.type = INPUT_LAYER_RUNTIME_PROFILER;
        input_layer_runtime_profiler.on_input = on_input_runtime_profiler;

        input_layer_memory_profiler.type = INPUT_LAYER_MEMORY_PROFILER;
        input_layer_memory_profiler.on_input = on_input_memory_profiler;

        push_input_layer(&input_layer_game);
    }
    
	window = os_window_create(1920, 1080, GAME_NAME, 0, 0);
	if (!window) {
		error("Failed to create window");
		return 1;
	}

	os_window_register_event_callback(window, on_window_event);

    if (!r_init_context(window)) {
        error("Failed to initialize render context");
        return 1;
    }

    r_detect_capabilities();

    r_init_buffer_storages();
    
    au_init_context();

    os_window_lock_cursor(window, true);
    os_set_vsync(false);

    stbi_set_flip_vertically_on_load(true);

    uniform_value_cache.data = allocl(MAX_UNIFORM_VALUE_CACHE_SIZE);
    uniform_value_cache.size = 0;
    uniform_value_cache.capacity = MAX_UNIFORM_VALUE_CACHE_SIZE;
    
    cache_texture_sids(&texture_sids);

    init_asset_source_table();
    init_asset_table();

#if DEVELOPER
    save_asset_pack(GAME_ASSET_PACK_PATH);
#endif
    load_asset_pack(GAME_ASSET_PACK_PATH);
        
    viewport.aspect_type = VIEWPORT_4X3;
    viewport.resolution_scale = 1.0f;
    
    const Texture_Format_Type color_attachments[] = { TEXTURE_FORMAT_RGB_8, TEXTURE_FORMAT_RED_INTEGER };
    r_init_frame_buffer(window->width, window->height,
                        color_attachments, COUNT(color_attachments),
                        TEXTURE_FORMAT_DEPTH_24_STENCIL_8,
                        &viewport.frame_buffer);

    viewport.frame_buffer.quantize_color_count = 32;
    
#if 0
    viewport.frame_buffer.pixel_size                  = 1.0f;
    viewport.frame_buffer.curve_distortion_factor     = 0.25f;
    viewport.frame_buffer.chromatic_aberration_offset = 0.002f;
    viewport.frame_buffer.quantize_color_count        = 16;
    viewport.frame_buffer.noise_blend_factor          = 0.1f;
    viewport.frame_buffer.scanline_count              = 64;
    viewport.frame_buffer.scanline_intensity          = 0.95f;
#endif
    
    resize_viewport(&viewport, window->width, window->height);

    ui_init();
    init_debug_console();
    init_runtime_profiler();
    
    init_render_queue(&entity_render_queue, MAX_RENDER_QUEUE_SIZE);
    geo_init();

    // @Cleanup: just make it better.
    extern void r_init_frame_buffer_draw();
    r_init_frame_buffer_draw();
    
	Hot_Reload_List hot_reload_list = {};
    // @Note: shader includes does not count as shader hot reload.
	register_hot_reload_directory(&hot_reload_list, DIR_SHADERS);
	register_hot_reload_directory(&hot_reload_list, DIR_TEXTURES);
	register_hot_reload_directory(&hot_reload_list, DIR_MATERIALS);
	register_hot_reload_directory(&hot_reload_list, DIR_MESHES);
	register_hot_reload_directory(&hot_reload_list, DIR_FLIP_BOOKS);

    const Thread hot_reload_thread = start_hot_reload_thread(&hot_reload_list);
    
    world = alloclt(World);
	init_world(world);

#if 1
    str_copy(world->name, "main.wl");

	auto &player = *(Player *)create_entity(world, ENTITY_PLAYER);
	{
        const auto &texture = asset_table.textures[texture_sids.player_idle[DIRECTION_BACK]];
        
        const f32 scale_aspect = (f32)texture.width / texture.height;
        const f32 y_scale = 1.0f * scale_aspect;
        const f32 x_scale = y_scale * scale_aspect;
        
		player.scale = vec3(x_scale, y_scale, 1.0f);
        player.location = vec3(0.0f, F32_MIN, 0.0f);

		player.draw_data.sid_mesh     = SID_MESH_PLAYER;
		player.draw_data.sid_material = SID_MATERIAL_PLAYER;
        player.draw_data.eid_vertex_data_offset = EID_VERTEX_DATA_SIZE;
        
        *(eid *)((u8 *)EID_VERTEX_DATA + EID_VERTEX_DATA_SIZE) = player.eid;
        EID_VERTEX_DATA_SIZE += sizeof(u32);
            
        const vec2 uv_scale = vec2(1.0f);
        auto &material = asset_table.materials[player.draw_data.sid_material];
		set_material_uniform_value(&material, "u_uv_scale", &uv_scale, sizeof(uv_scale));
        
        player.sid_sound_steps = SID_SOUND_PLAYER_STEPS;
	}

    auto &portal = *(Portal *)create_entity(world, ENTITY_PORTAL);
    {        
        portal.destination_location = vec3(0.0f, F32_MIN, 0.0f);
        
        portal.scale = vec3(1.0f);
        portal.location = vec3(-3.0f, 0.0f, 3.0f);

        auto &aabb = world->aabbs[portal.aabb_index];
        const vec3 aabb_offset = portal.scale * 2;
        aabb.min = portal.location - aabb_offset * 0.5f;
		aabb.max = aabb.min + aabb_offset;
    }
    
	auto &ground = *(Static_Mesh *)create_entity(world, ENTITY_STATIC_MESH);
	{        
		ground.scale = vec3(16.0f, 16.0f, 0.0f);
        ground.rotation = quat_from_axis_angle(vec3_right, 90.0f);

        auto &aabb = world->aabbs[ground.aabb_index];
        const vec3 aabb_offset = vec3(ground.scale.x * 2, 0.0f, ground.scale.y * 2);
        aabb.min = ground.location - aabb_offset * 0.5f;
		aabb.max = aabb.min + aabb_offset;

        ground.draw_data.sid_mesh     = SID_MESH_QUAD;
        ground.draw_data.sid_material = SID_MATERIAL_GROUND;
        ground.draw_data.eid_vertex_data_offset = EID_VERTEX_DATA_SIZE;
        
        *(eid *)((u8 *)EID_VERTEX_DATA + EID_VERTEX_DATA_SIZE) = ground.eid;
        EID_VERTEX_DATA_SIZE += sizeof(u32);

        auto &material = asset_table.materials[ground.draw_data.sid_material];
        const vec2 uv_scale = vec2(ground.scale.x, ground.scale.y);
		set_material_uniform_value(&material, "u_uv_scale", &uv_scale, sizeof(uv_scale));
	}

	auto &cube = *(Static_Mesh *)create_entity(world, ENTITY_STATIC_MESH);
	{                
		cube.location = vec3(3.0f, 0.5f, 4.0f);

        auto &aabb = world->aabbs[cube.aabb_index];
		aabb.min = cube.location - cube.scale * 0.5f;
		aabb.max = aabb.min + cube.scale;

		cube.draw_data.sid_mesh     = SID_MESH_CUBE;
		cube.draw_data.sid_material = SID_MATERIAL_CUBE;
        cube.draw_data.eid_vertex_data_offset = EID_VERTEX_DATA_SIZE;
        
        *(eid *)((u8 *)EID_VERTEX_DATA + EID_VERTEX_DATA_SIZE) = cube.eid;
        EID_VERTEX_DATA_SIZE += sizeof(u32);
        
        const vec2 uv_scale = vec2(1.0f);
        auto &material = asset_table.materials[cube.draw_data.sid_material];
		set_material_uniform_value(&material, "u_uv_scale", &uv_scale, sizeof(uv_scale));
	}

	auto &skybox = *(Skybox *)create_entity(world, ENTITY_SKYBOX);
	{
        skybox.location = vec3(0.0f, -2.0f, 0.0f);
        skybox.uv_scale = vec2(8.0f, 4.0f);
        
		skybox.draw_data.sid_mesh     = SID_MESH_SKYBOX;
		skybox.draw_data.sid_material = SID_MATERIAL_SKYBOX;
        skybox.draw_data.eid_vertex_data_offset = EID_VERTEX_DATA_SIZE;
        
        *(eid *)((u8 *)EID_VERTEX_DATA + EID_VERTEX_DATA_SIZE) = skybox.eid;
        EID_VERTEX_DATA_SIZE += sizeof(u32);
	}

    auto &sound_emitter_2d = *(Sound_Emitter_2D *)create_entity(world, ENTITY_SOUND_EMITTER_2D);
    {
        sound_emitter_2d.location = vec3(0.0f, 2.0f, 0.0f);
        sound_emitter_2d.sid_sound = SID_SOUND_WIND_AMBIENCE;
    }
    
    if (1) {
        auto &direct_light = *(Direct_Light *)create_entity(world, ENTITY_DIRECT_LIGHT);

        direct_light.location = vec3(0.0f, 5.0f, 0.0f);
        direct_light.rotation = quat_from_axis_angle(vec3_right, 0.0f);
        direct_light.scale = vec3(0.1f);
        
        direct_light.ambient  = vec3_black;
        direct_light.diffuse  = vec3_black;
        direct_light.specular = vec3_black;

        direct_light.u_light_index = 0;
        
        auto &aabb = world->aabbs[direct_light.aabb_index];
		aabb.min = direct_light.location - direct_light.scale * 0.5f;
		aabb.max = aabb.min + direct_light.scale;
    }
    
    if (1) {
        auto &point_light = *(Point_Light *)create_entity(world, ENTITY_POINT_LIGHT);
        
        point_light.location = vec3(0.0f, 2.0f, 0.0f);
        point_light.scale = vec3(0.1f);
        
        point_light.ambient  = vec3(0.1f);
        point_light.diffuse  = vec3(0.5f);
        point_light.specular = vec3(1.0f);

        point_light.attenuation.constant  = 1.0f;
        point_light.attenuation.linear    = 0.09f;
        point_light.attenuation.quadratic = 0.032f;
        
        point_light.u_light_index = 0;
        
        auto &aabb = world->aabbs[point_light.aabb_index];
		aabb.min = point_light.location - point_light.scale * 0.5f;
		aabb.max = aabb.min + point_light.scale;
    }

    auto &camera = world->camera;
	camera.mode = MODE_PERSPECTIVE;
	camera.yaw = 90.0f;
	camera.pitch = 0.0f;
	camera.eye = player.location + player.camera_offset;
	camera.at = camera.eye + forward(camera.yaw, camera.pitch);
	camera.up = vec3(0.0f, 1.0f, 0.0f);
	camera.fov = 60.0f;
	camera.near = 0.001f;
	camera.far = 1000.0f;
	camera.left = 0.0f;
	camera.right = (f32)window->width;
	camera.bottom = 0.0f;
	camera.top = (f32)window->height;

	world->ed_camera = camera;

    save_world_level(world);
#else
    const char *main_level_path = PATH_LEVEL("main.wl");
    load_world_level(world, main_level_path);    
#endif
      
    {
        RID_UNIFORM_BUFFER = r_create_uniform_buffer(MAX_UNIFORM_BUFFER_SIZE);
        
        constexpr s32 MAX_UNIFORM_LIGHTS = 64; // must be the same as in shaders
        static_assert(MAX_UNIFORM_LIGHTS >= MAX_POINT_LIGHTS + MAX_DIRECT_LIGHTS);

        const Uniform_Block_Field camera_fields[] = {
            { UNIFORM_F32_3,   1 },
            { UNIFORM_F32_4X4, 1 },
            { UNIFORM_F32_4X4, 1 },
            { UNIFORM_F32_4X4, 1 },
        };

        const Uniform_Block_Field viewport_fields[] = {
            { UNIFORM_F32_2,   1 },
            { UNIFORM_F32_4X4, 1 },
        };
         
        const Uniform_Block_Field direct_light_fields[] = {
            { UNIFORM_U32,   1 },
            { UNIFORM_F32_3, MAX_DIRECT_LIGHTS },
            { UNIFORM_F32_3, MAX_DIRECT_LIGHTS },
            { UNIFORM_F32_3, MAX_DIRECT_LIGHTS },
            { UNIFORM_F32_3, MAX_DIRECT_LIGHTS },
        };

        const Uniform_Block_Field point_light_fields[] = {
            { UNIFORM_U32,   1 },
            { UNIFORM_F32_3, MAX_POINT_LIGHTS },
            { UNIFORM_F32_3, MAX_POINT_LIGHTS },
            { UNIFORM_F32_3, MAX_POINT_LIGHTS },
            { UNIFORM_F32_3, MAX_POINT_LIGHTS },
            { UNIFORM_F32,   MAX_POINT_LIGHTS },
            { UNIFORM_F32,   MAX_POINT_LIGHTS },
            { UNIFORM_F32,   MAX_POINT_LIGHTS },
        };

        r_add_uniform_block(RID_UNIFORM_BUFFER,
                            UNIFORM_BLOCK_BINDING_CAMERA,
                            UNIFORM_BLOCK_NAME_CAMERA,
                            camera_fields, COUNT(camera_fields),
                            &uniform_block_camera);

        r_add_uniform_block(RID_UNIFORM_BUFFER,
                            UNIFORM_BLOCK_BINDING_VIEWPORT,
                            UNIFORM_BLOCK_NAME_VIEWPORT,
                            viewport_fields, COUNT(viewport_fields),
                            &uniform_block_viewport);

        r_add_uniform_block(RID_UNIFORM_BUFFER,
                            UNIFORM_BLOCK_BINDING_DIRECT_LIGHTS,
                            UNIFORM_BLOCK_NAME_DIRECT_LIGHTS,
                            direct_light_fields, COUNT(direct_light_fields),
                            &uniform_block_direct_lights);

        r_add_uniform_block(RID_UNIFORM_BUFFER,
                            UNIFORM_BLOCK_BINDING_POINT_LIGHTS,
                            UNIFORM_BLOCK_NAME_POINT_LIGHTS,
                            point_light_fields, COUNT(point_light_fields),
                            &uniform_block_point_lights);
    }

	delta_time = 0.0f;
	s64 begin_counter = os_perf_counter();

    log("Startup took %.2fms", CHECK_SCOPE_TIMER_MS(startup));
            
	while (os_window_is_alive(window)) {
        PROFILE_SCOPE("game_frame");

		os_window_poll_events(window);

        // @Cleanup: this one is pretty slow, but bearable for now.
        // Move to other thread later if it becomes a big deal.
        check_for_hot_reload(&hot_reload_list);

        tick_game(delta_time);
        tick_editor(delta_time);

#if 0
        static f32 pixel_size_time = -1.0f;
        if (pixel_size_time > 180.0f) pixel_size_time = 0.0f;
        viewport.frame_buffer.pixel_size = (sin(pixel_size_time) + 1.0f) * viewport.frame_buffer.width * 0.05f;
        pixel_size_time += delta_time * 4.0f;
#endif
        
        Render_Command frame_buffer_command = {};
        frame_buffer_command.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR;
        frame_buffer_command.viewport.x = 0;
        frame_buffer_command.viewport.y = 0;
        frame_buffer_command.viewport.width  = viewport.frame_buffer.width;
        frame_buffer_command.viewport.height = viewport.frame_buffer.height;
        frame_buffer_command.scissor.x = 0;
        frame_buffer_command.scissor.y = 0;
        frame_buffer_command.scissor.width  = viewport.frame_buffer.width;
        frame_buffer_command.scissor.height = viewport.frame_buffer.height;
        frame_buffer_command.rid_frame_buffer = viewport.frame_buffer.rid;
        r_submit(&frame_buffer_command);

        {
            Render_Command command = {};
            command.flags = RENDER_FLAG_CLEAR;
            command.clear.color = vec3_white;
            command.clear.flags = CLEAR_FLAG_COLOR | CLEAR_FLAG_DEPTH | CLEAR_FLAG_STENCIL;
            r_submit(&command);
        }
        
        draw_world(world);

#if DEVELOPER
        geo_draw_debug();
        draw_dev_stats();
        draw_debug_console();
        draw_runtime_profiler();
        draw_memory_profiler();
#endif
        
        update_render_stats();
        
		r_flush(&entity_render_queue);
        geo_flush();
         
        frame_buffer_command.flags = RENDER_FLAG_RESET;
        r_submit(&frame_buffer_command);

        {
            Render_Command command = {};
            command.flags = RENDER_FLAG_CLEAR;
            command.clear.color = vec3_black;
            command.clear.flags = CLEAR_FLAG_COLOR;
            r_submit(&command);
        }
        
        draw_frame_buffer(&viewport.frame_buffer, 0);
        ui_flush();

		os_window_swap_buffers(window);
        
        freef(); // clear frame allocation
 
		const s64 end_counter = os_perf_counter();
		delta_time = (end_counter - begin_counter) / (f32)os_perf_frequency_s();
		begin_counter = end_counter;
        
#if DEVELOPER
        // If dt is too large, we could have resumed from a breakpoint.
        if (delta_time > 1.0f) {
            delta_time = 0.16f;
        }
#endif
	}

    os_thread_terminate(hot_reload_thread);
	os_window_destroy(window);
    alloc_shutdown();
    
	return 0;
}
