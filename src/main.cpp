#include "pch.h"
#include "log.h"
#include "str.h"
#include "sid.h"
#include "font.h"
#include "flip_book.h"
#include "profile.h"
#include "asset.h"
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
#include "render/render_registry.h"
#include "render/geometry_draw.h"
#include "render/viewport.h"
#include "render/ui.h"

#include "audio/audio_registry.h"

#include "game/world.h"
#include "game/game.h"

#include "editor/hot_reload.h"
#include "editor/debug_console.h"

static void fill_cube_vertices(Vertex_Entity *vertices, s32 entity_id) {
    s32 i = 0;
    
    // Front face
    vertices[i++] = { vec3(-0.5f,  0.5f,  0.5f), vec3_forward, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f,  0.5f,  0.5f), vec3_forward, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f, -0.5f,  0.5f), vec3_forward, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f,  0.5f), vec3_forward, vec2(1.0f, 1.0f), entity_id };

    // Back face
    vertices[i++] = { vec3(-0.5f,  0.5f, -0.5f), vec3_back, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f,  0.5f, -0.5f), vec3_back, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f, -0.5f, -0.5f), vec3_back, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f, -0.5f), vec3_back, vec2(1.0f, 1.0f), entity_id };

    // Left face
    vertices[i++] = { vec3(-0.5f,  0.5f, -0.5f), vec3_left, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f,  0.5f,  0.5f), vec3_left, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f, -0.5f, -0.5f), vec3_left, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3(-0.5f, -0.5f,  0.5f), vec3_left, vec2(1.0f, 1.0f), entity_id };

    // Right face
    vertices[i++] = { vec3( 0.5f,  0.5f, -0.5f), vec3_right, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f,  0.5f,  0.5f), vec3_right, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f, -0.5f), vec3_right, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f,  0.5f), vec3_right, vec2(1.0f, 1.0f), entity_id };

    // Top face
    vertices[i++] = { vec3(-0.5f,  0.5f, -0.5f), vec3_up, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f,  0.5f, -0.5f), vec3_up, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f,  0.5f,  0.5f), vec3_up, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3( 0.5f,  0.5f,  0.5f), vec3_up, vec2(1.0f, 1.0f), entity_id };

    // Bottom face
    vertices[i++] = { vec3(-0.5f, -0.5f, -0.5f), vec3_down, vec2(0.0f, 0.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f, -0.5f), vec3_down, vec2(1.0f, 0.0f), entity_id };
    vertices[i++] = { vec3(-0.5f, -0.5f,  0.5f), vec3_down, vec2(0.0f, 1.0f), entity_id };
    vertices[i++] = { vec3( 0.5f, -0.5f,  0.5f), vec3_down, vec2(1.0f, 1.0f), entity_id };
}

s32 main() {
    PROFILE_START(startup, "Startup");
    START_SCOPE_TIMER(startup);

    if (!alloc_init()) {
        error("Failed to initialize allocation");
        return 1;
    }

    init_sid_table();
	init_input_table();
    
	window = create_window(1280, 720, GAME_NAME, 0, 0);
	if (!window) {
		error("Failed to create window");
		return 1;
	}

	register_event_callback(window, on_window_event);

    if (!init_render_context(window)) {
        error("Failed to initialize render context");
        return 1;
    }

    detect_render_capabilities();

    init_audio_context();

    lock_cursor(window, true);
    set_vsync(false);
    stbi_set_flip_vertically_on_load(true);
    
    init_render_registry(&render_registry);
    init_audio_registry();

    cache_shader_sids(&shader_sids);
    cache_texture_sids(&texture_sids);
    cache_sound_sids(&sound_sids);

    init_asset_source_table(&asset_source_table);
    init_asset_table(&asset_table);
    
    save_asset_pack(GAME_ASSET_PACK_PATH);
    load_asset_pack(GAME_ASSET_PACK_PATH, &asset_table);
    
    // @Cleanup: move these to asset pak?
	create_game_materials(&material_index_list);
	create_game_flip_books(&flip_books);
    
    viewport.aspect_type = VIEWPORT_4X3;
    viewport.resolution_scale = 1.0f;


    const Texture_Format_Type color_attachments[] = { TEXTURE_FORMAT_RGB_8, TEXTURE_FORMAT_RED_INTEGER };
    viewport.frame_buffer_index = create_frame_buffer(window->width, window->height,
                                                      color_attachments, COUNT(color_attachments),
                                                      TEXTURE_FORMAT_DEPTH_24_STENCIL_8);

    auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];

    viewport_frame_buffer.quantize_color_count = 32;
    
#if 0
    viewport_frame_buffer.pixel_size                  = 1.0f;
    viewport_frame_buffer.curve_distortion_factor     = 0.25f;
    viewport_frame_buffer.chromatic_aberration_offset = 0.002f;
    viewport_frame_buffer.quantize_color_count        = 16;
    viewport_frame_buffer.noise_blend_factor          = 0.1f;
    viewport_frame_buffer.scanline_count              = 64;
    viewport_frame_buffer.scanline_intensity          = 0.95f;
#endif
    
    resize_viewport(&viewport, window->width, window->height);

    ui_init();
    init_debug_console();

    init_render_queue(&entity_render_queue, MAX_RENDER_QUEUE_SIZE);
    init_geo_draw();

	Hot_Reload_List hot_reload_list = {};
    // @Note: shader includes does not count as shader hot reload.
	register_hot_reload_directory(&hot_reload_list, DIR_SHADERS);
	register_hot_reload_directory(&hot_reload_list, DIR_TEXTURES);
	register_hot_reload_directory(&hot_reload_list, DIR_SOUNDS);
    
    world = alloclt(World);
	init_world(world);

#if 1
    str_copy(world->name, "main");

	auto &player = world->player;
	{   // Create player.
        player.id = 1;

        const auto *asset = asset_table.find(texture_sids.player_idle[DIRECTION_BACK]);
        const auto &texture = render_registry.textures[asset->registry_index];
        
        const f32 scale_aspect = (f32)texture.width / texture.height;
        const f32 y_scale = 1.0f * scale_aspect;
        const f32 x_scale = y_scale * scale_aspect;

        player.aabb_index = world->aabbs.add_default();
        
		player.scale = vec3(x_scale, y_scale, 1.0f);
        player.location = vec3(0.0f, F32_MIN, 0.0f);

		player.draw_data.material_index = material_index_list.player;

        static const vec3 uv_scale = vec3(1.0f);
		set_material_uniform_value(player.draw_data.material_index, "u_uv_scale", &uv_scale);
        
        // Little uv offset as source textures have small transient border.
        const f32 uv_offset = 0.02f;
		const Vertex_Entity vertices[4] = { // center in bottom mid point of quad
			{ vec3( 0.5f,  1.0f, 0.0f), vec3_up, vec2(1.0f - uv_offset, 1.0f - uv_offset), player.id },
            { vec3( 0.5f,  0.0f, 0.0f), vec3_up, vec2(1.0f - uv_offset, 0.0f + uv_offset), player.id },
            { vec3(-0.5f,  0.0f, 0.0f), vec3_up, vec2(0.0f + uv_offset, 0.0f + uv_offset), player.id },
            { vec3(-0.5f,  1.0f, 0.0f), vec3_up, vec2(0.0f + uv_offset, 1.0f - uv_offset), player.id },
		};
        
        Vertex_Array_Binding binding = {};
        binding.layout_size = COUNT(vertex_entity_layout);
        copy_bytes(binding.layout, vertex_entity_layout, sizeof(vertex_entity_layout));
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
        
		player.draw_data.vertex_array_index = create_vertex_array(&binding, 1);

		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		player.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);

        player.steps_sid = sound_sids.player_steps;
	}

	auto &ground = world->static_meshes[create_static_mesh(world)];
	{   // Create ground.
        ground.id = 10;
        
		ground.scale = vec3(32.0f, 32.0f, 0.0f);
        ground.rotation = quat_from_axis_angle(vec3_right, 90.0f);

        auto &aabb = world->aabbs[ground.aabb_index];
        const vec3 aabb_offset = vec3(ground.scale.x, 0.0f, ground.scale.y);
        aabb.min = ground.location - aabb_offset * 0.5f;
		aabb.max = aabb.min + aabb_offset;

        ground.draw_data.material_index = material_index_list.ground;
		set_material_uniform_value(ground.draw_data.material_index, "u_uv_scale", &ground.scale);

		const Vertex_Entity vertices[] = {
			{ vec3( 0.5f,  0.5f, 0.0f), vec3_up, vec2(1.0f, 1.0f), ground.id },
            { vec3( 0.5f, -0.5f, 0.0f), vec3_up, vec2(1.0f, 0.0f), ground.id },
            { vec3(-0.5f, -0.5f, 0.0f), vec3_up, vec2(0.0f, 0.0f), ground.id },
            { vec3(-0.5f,  0.5f, 0.0f), vec3_up, vec2(0.0f, 1.0f), ground.id },
		};
        
        Vertex_Array_Binding binding = {};
        binding.layout_size = COUNT(vertex_entity_layout);
        copy_bytes(binding.layout, vertex_entity_layout, sizeof(vertex_entity_layout));
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
        
		ground.draw_data.vertex_array_index = create_vertex_array(&binding, 1);

		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		ground.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}

	auto &cube = world->static_meshes[create_static_mesh(world)];
	{   // Create cube.
        cube.id = 20;
                
		cube.location = vec3(3.0f, 0.5f, 4.0f);

        auto &aabb = world->aabbs[cube.aabb_index];
		aabb.min = cube.location - cube.scale * 0.5f;
		aabb.max = aabb.min + cube.scale;

		cube.draw_data.material_index = material_index_list.cube;

        const vec3 uv_scale = vec3(1.0f);
		set_material_uniform_value(cube.draw_data.material_index, "u_uv_scale", &uv_scale);
        
		Vertex_Entity vertices[24];
        fill_cube_vertices(vertices, cube.id);
        
        Vertex_Array_Binding binding = {};
        binding.layout_size = COUNT(vertex_entity_layout);
        copy_bytes(binding.layout, vertex_entity_layout, sizeof(vertex_entity_layout));
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
                
		cube.draw_data.vertex_array_index = create_vertex_array(&binding, 1);

		const u32 indices[] = {
			// Front face
			0,   1,  2,  1,  3,  2,
			// Back face
			4,   6,  5,  5,  6,  7,
			// Left face
			8,   9, 10,  9, 11, 10,
			// Right face
			12, 14, 13, 13, 14, 15,
			// Bottom face
			16, 17, 18, 17, 19, 18,
			// Top face
			20, 22, 21, 21, 22, 23
		};
        
		cube.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}

	auto &skybox = world->skybox;
	{   // Create skybox.
        skybox.id = S32_MAX;
        skybox.uv_scale = vec2(8.0f, 4.0f);
        
		skybox.draw_data.material_index = material_index_list.skybox;

		Vertex_Entity vertices[] = {
			{ vec3( 1.0f,  1.0f, 1.0f - F32_EPSILON), vec3_zero, vec2(1.0f, 1.0f), skybox.id },
            { vec3( 1.0f, -1.0f, 1.0f - F32_EPSILON), vec3_zero, vec2(1.0f, 0.0f), skybox.id },
            { vec3(-1.0f, -1.0f, 1.0f - F32_EPSILON), vec3_zero, vec2(0.0f, 0.0f), skybox.id },
            { vec3(-1.0f,  1.0f, 1.0f - F32_EPSILON), vec3_zero, vec2(0.0f, 1.0f), skybox.id },
		};

        Vertex_Array_Binding binding = {};
        binding.layout_size = COUNT(vertex_entity_layout);
        copy_bytes(binding.layout, vertex_entity_layout, sizeof(vertex_entity_layout));
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
                
		skybox.draw_data.vertex_array_index = create_vertex_array(&binding, 1);
        
		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		skybox.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}
    
    
    if (1) {
        const s32 index = world->direct_lights.add_default();
        
        auto &direct_light = world->direct_lights[index];
        direct_light.id = 20000;

        direct_light.location = vec3(0.0f, 5.0f, 0.0f);
        direct_light.rotation = quat_from_axis_angle(vec3_right, 0.0f);
        direct_light.scale = vec3(0.1f);
        
        direct_light.ambient  = vec3_black;
        direct_light.diffuse  = vec3_black;
        direct_light.specular = vec3_black;

        direct_light.u_light_index = index;
        direct_light.aabb_index = world->aabbs.add_default();
        
        auto &aabb = world->aabbs[direct_light.aabb_index];
		aabb.min = direct_light.location - direct_light.scale * 0.5f;
		aabb.max = aabb.min + direct_light.scale;
    }
    
    if (1) {
        const s32 index = world->point_lights.add_default();
        
        auto &point_light = world->point_lights[index];
        point_light.id = 10000;
        
        point_light.location = vec3(0.0f, 2.0f, 0.0f);
        point_light.scale = vec3(0.1f);
        
        point_light.ambient  = vec3(0.1f);
        point_light.diffuse  = vec3(0.5f);
        point_light.specular = vec3(1.0f);

        point_light.attenuation.constant  = 1.0f;
        point_light.attenuation.linear    = 0.09f;
        point_light.attenuation.quadratic = 0.032f;
        
        point_light.u_light_index = index;
        point_light.aabb_index = world->aabbs.add_default();
        
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

    save_world(world);
#else
    load_world(world, "C:/dev/scopecorp/run_tree/data/levels/main.wl");    
#endif
      
    {
        constexpr u32 UNIFORM_BUFFER_SIZE = KB(16);
        const s32 ubi = create_uniform_buffer(UNIFORM_BUFFER_SIZE);
        
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

        UNIFORM_BLOCK_CAMERA = create_uniform_block(ubi, UNIFORM_BINDING_CAMERA, UNIFORM_BLOCK_NAME_CAMERA, camera_fields, COUNT(camera_fields));

        UNIFORM_BLOCK_VIEWPORT = create_uniform_block(ubi, UNIFORM_BINDING_VIEWPORT, UNIFORM_BLOCK_NAME_VIEWPORT, viewport_fields, COUNT(viewport_fields));
        
        UNIFORM_BLOCK_DIRECT_LIGHTS = create_uniform_block(ubi, UNIFORM_BINDING_DIRECT_LIGHTS, UNIFORM_BLOCK_NAME_DIRECT_LIGHTS, direct_light_fields, COUNT(direct_light_fields));
        
        UNIFORM_BLOCK_POINT_LIGHTS = create_uniform_block(ubi, UNIFORM_BINDING_POINT_LIGHTS, UNIFORM_BLOCK_NAME_POINT_LIGHTS, point_light_fields, COUNT(point_light_fields));
    }

	delta_time = 0.0f;
	s64 begin_counter = performance_counter();

    log("Startup took %.2fms", CHECK_SCOPE_TIMER_MS(startup));
    PROFILE_END(startup);
    
	while (alive(window)) {
        PROFILE_SCOPE("game_frame");

		poll_events(window);
        tick(world, delta_time);        
		//set_listener_pos(player.location);

        // @Cleanup: this one is pretty slow, but bearable for now (~0.2ms);
        // move to other thread later if it becomes a big deal.
        check_for_hot_reload(&hot_reload_list);

#if 0
        static f32 pixel_size_time = -1.0f;
        if (pixel_size_time > 180.0f) pixel_size_time = 0.0f;
        viewport_frame_buffer.pixel_size = (sin(pixel_size_time) + 1.0f) * viewport_frame_buffer.width * 0.05f;
        pixel_size_time += delta_time * 4.0f;
#endif
        
        Render_Command frame_buffer_command = {};
        frame_buffer_command.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR;
        frame_buffer_command.viewport.x = 0;
        frame_buffer_command.viewport.y = 0;
        frame_buffer_command.viewport.width  = viewport_frame_buffer.width;
        frame_buffer_command.viewport.height = viewport_frame_buffer.height;
        frame_buffer_command.scissor.x = 0;
        frame_buffer_command.scissor.y = 0;
        frame_buffer_command.scissor.width  = viewport_frame_buffer.width;
        frame_buffer_command.scissor.height = viewport_frame_buffer.height;
        frame_buffer_command.frame_buffer_index = viewport.frame_buffer_index;
        submit(&frame_buffer_command);

        {
            Render_Command command = {};
            command.flags = RENDER_FLAG_CLEAR;
            command.clear.color = vec3_white;
            command.clear.flags = CLEAR_FLAG_COLOR | CLEAR_FLAG_DEPTH | CLEAR_FLAG_STENCIL;
            submit(&command);
        }
        
        draw_world(world);

#if DEVELOPER
        draw_geo_debug();
        draw_dev_stats();
        draw_debug_console();
#endif
        
        update_render_stats();
        
		flush(&entity_render_queue);
        flush_geo_draw();
         
        frame_buffer_command.flags = RENDER_FLAG_RESET;
        submit(&frame_buffer_command);

        {
            Render_Command command = {};
            command.flags = RENDER_FLAG_CLEAR;
            command.clear.color = vec3_black;
            command.clear.flags = CLEAR_FLAG_COLOR;
            submit(&command);
        }
        
        draw_frame_buffer(viewport.frame_buffer_index, 0);
        ui_flush();

		swap_buffers(window);
        PROFILE_FRAME("Game Frame");
        
        freef(); // clear frame allocation
 
		const s64 end_counter = performance_counter();
		delta_time = (end_counter - begin_counter) / (f32)performance_frequency_s();
		begin_counter = end_counter;
        
#if DEVELOPER
        // If dt is too large, we could have resumed from a breakpoint.
        if (delta_time > 1.0f) delta_time = 0.16f;
#endif
	}

	destroy(window);
    alloc_shutdown();
    
	return 0;
}
