#include "pch.h"
#include "log.h"
#include "sid.h"
#include "font.h"
#include "flip_book.h"
#include "memory_storage.h"
#include "profile.h"
#include "asset_registry.h"
#include "stb_sprintf.h"
#include "stb_image.h"

#include "os/thread.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"
#include "os/file.h"

#include "math/math_core.h"

#include "render/viewport.h"
#include "render/render_command.h"
#include "render/render_registry.h"
#include "render/render_stats.h"
#include "render/geometry_draw.h"
#include "render/text.h"

#include "audio/audio_registry.h"

#include "game/world.h"
#include "game/game.h"

#include "editor/hot_reload.h"

s32 main() {
    PROFILE_START(startup, "Startup");
    START_SCOPE_TIMER(startup);
    
    void *vm = allocate_core();
    if (!vm) return 1;
    
	log("Preallocated memory storages: Persistent %.fmb | Frame %.fmb | Temp %.fmb",
		(f32)pers_memory_size / 1024 / 1024, (f32)frame_memory_size / 1024 / 1024, (f32)temp_memory_size / 1024 / 1024);

    init_sid_table(&sid_table);
    
	window = create_window(1280, 720, GAME_NAME, 0, 0);
	if (!window) {
		error("Failed to create window");
		return 1;
	}

	register_event_callback(window, on_window_event);

	init_input_table();
    init_render_context(window);
    init_audio_context();

    lock_cursor(window, true);
    set_vsync(false);
    stbi_set_flip_vertically_on_load(true);
    
    init_render_registry(&render_registry);
    init_audio_registry();

    init_asset_sources(&asset_sources);
    init_asset_table(&asset_table);
    
    save_asset_pack(PACK_PATH("test.pack"));
    load_asset_pack(PACK_PATH("test.pack"), &asset_table);

    cache_shader_sids(&shader_sids);
    cache_texture_sids(&texture_sids);
    cache_sound_sids(&sound_sids);
    
	create_game_materials(&material_index_list);
	create_game_flip_books(&flip_books);
    
    viewport.aspect_type = VIEWPORT_4X3;
    viewport.resolution_scale = 1.0f;
    
    const Texture_Format_Type color_attachments[] = { TEXTURE_FORMAT_RGB_8, TEXTURE_FORMAT_RED_INTEGER };
    viewport.frame_buffer_index = create_frame_buffer(window->width, window->height,
                                                      color_attachments, COUNT(color_attachments),
                                                      TEXTURE_FORMAT_DEPTH_24_STENCIL_8);

    auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];

#if 0
    viewport_frame_buffer.pixel_size                  = 1.0f;
    viewport_frame_buffer.curve_distortion_factor     = 0.25f;
    viewport_frame_buffer.chromatic_aberration_offset = 0.002f;
    viewport_frame_buffer.quantize_color_count        = 16;
    viewport_frame_buffer.noise_blend_factor          = 0.3f;
    viewport_frame_buffer.scanline_count              = 16;
    viewport_frame_buffer.scanline_intensity          = 0.9f;
#endif
    
	Hot_Reload_List hot_reload_list = {};
	register_hot_reload_dir(&hot_reload_list, SHADER_FOLDER, on_shader_changed_externally);
	start_hot_reload_thread(&hot_reload_list);

	Font *font = create_font(FONT_PATH("consola.ttf"));
	Font_Atlas *atlas = bake_font_atlas(font, 33, 126, 16);
    
	world = push_struct(pers, World);
	init_world(world);

    init_render_queue(&entity_render_queue, MAX_RENDER_QUEUE_SIZE);
    init_text_draw(atlas);
    init_geo_draw();

	Player &player = world->player;
	{   // Create player.
        player.id = 1;

        const Asset *asset = asset_table.find(texture_sids.player_idle[DIRECTION_BACK]);
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
			{ vec3( 0.5f,  1.0f, 0.0f), vec2(1.0f - uv_offset, 1.0f - uv_offset), player.id },
			{ vec3( 0.5f,  0.0f, 0.0f), vec2(1.0f - uv_offset, 0.0f + uv_offset), player.id },
			{ vec3(-0.5f,  0.0f, 0.0f), vec2(0.0f + uv_offset, 0.0f + uv_offset), player.id },
			{ vec3(-0.5f,  1.0f, 0.0f), vec2(0.0f + uv_offset, 1.0f - uv_offset), player.id },
		};
        
        Vertex_Array_Binding binding = {};
        binding.layout_size = 3;
        binding.layout[0] = { VERTEX_F32_3, 0 };
        binding.layout[1] = { VERTEX_F32_2, 0 };
        binding.layout[2] = { VERTEX_S32,   1 };
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
        
		player.draw_data.vertex_array_index = create_vertex_array(&binding, 1);

		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		player.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &ground = world->static_meshes[create_static_mesh(world)];
	{   // Create ground.
        ground.id = 10;
        
		ground.scale = vec3(32.0f, 32.0f, 0.0f);
        ground.rotation = quat_from_axis_angle(vec3_right, 90.0f);

        auto &aabb = world->aabbs[ground.aabb_index];
        const vec3 aabb_offset = vec3(16.0f, 0.0f, 16.0f);
        aabb.min = ground.location - aabb_offset * 0.5f;
		aabb.max = aabb.min + aabb_offset;

        ground.draw_data.material_index = material_index_list.ground;
		set_material_uniform_value(ground.draw_data.material_index, "u_uv_scale", &ground.scale);

		const Vertex_Entity vertices[] = {
			{ vec3( 0.5f,  0.5f, 0.0f), vec2(1.0f, 1.0f), ground.id },
			{ vec3( 0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f), ground.id },
			{ vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 0.0f), ground.id },
			{ vec3(-0.5f,  0.5f, 0.0f), vec2(0.0f, 1.0f), ground.id },
		};
        
        Vertex_Array_Binding binding = {};
        binding.layout_size = 3;
        binding.layout[0] = { VERTEX_F32_3, 0 };
        binding.layout[1] = { VERTEX_F32_2, 0 };
        binding.layout[2] = { VERTEX_S32,   1 };
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
        
		ground.draw_data.vertex_array_index = create_vertex_array(&binding, 1);

		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		ground.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &cube = world->static_meshes[create_static_mesh(world)];
	{   // Create cube.
        cube.id = 20;
                
		cube.location = vec3(3.0f, 0.5f, 4.0f);

        auto &aabb = world->aabbs[cube.aabb_index];
		aabb.min = cube.location - cube.scale * 0.5f;
		aabb.max = aabb.min + cube.scale;

		cube.draw_data.material_index = material_index_list.cube;

        const vec3 uv_scale = vec3(1.0f);
		set_material_uniform_value(cube.draw_data.material_index, "u_uv_scale", &uv_scale);
        
		const Vertex_Entity vertices[] = {
			// Front face
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f), cube.id },

			// Back face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 1.0f), cube.id },

			// Left face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f), cube.id },

			// Right face
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f), cube.id },

			// Top face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 1.0f), cube.id },

			// Bottom face
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 0.0f), cube.id },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 0.0f), cube.id },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f), cube.id },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f), cube.id },
		};

        Vertex_Array_Binding binding = {};
        binding.layout_size = 3;
        binding.layout[0] = { VERTEX_F32_3, 0 };
        binding.layout[1] = { VERTEX_F32_2, 0 };
        binding.layout[2] = { VERTEX_S32,   1 };
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

	Skybox &skybox = world->skybox;
	{   // Create skybox.
        skybox.id = 999;

		skybox.draw_data.material_index = material_index_list.skybox;

		Vertex_Entity vertices[] = {
			{ vec3( 1.0f,  1.0f, 1.0f - F32_EPSILON), vec2(1.0f, 1.0f), skybox.id },
			{ vec3( 1.0f, -1.0f, 1.0f - F32_EPSILON), vec2(1.0f, 0.0f), skybox.id },
			{ vec3(-1.0f, -1.0f, 1.0f - F32_EPSILON), vec2(0.0f, 0.0f), skybox.id },
			{ vec3(-1.0f,  1.0f, 1.0f - F32_EPSILON), vec2(0.0f, 1.0f), skybox.id },
		};

        Vertex_Array_Binding binding = {};
        binding.layout_size = 3;
        binding.layout[0] = { VERTEX_F32_3, 0 };
        binding.layout[1] = { VERTEX_F32_2, 0 };
        binding.layout[2] = { VERTEX_S32,   1 };
        binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Entity), BUFFER_USAGE_STATIC);
                
		skybox.draw_data.vertex_array_index = create_vertex_array(&binding, 1);
        
		const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		skybox.draw_data.index_buffer_index = create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
	}

	Camera &camera = world->camera;
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

	delta_time = 0.0f;
	s64 begin_counter = performance_counter();

    log("Startup took %.2fms", CHECK_SCOPE_TIMER_MS(startup));
    PROFILE_END(startup);
    
	while (alive(window)) {
        PROFILE_SCOPE("Game Frame");

		poll_events(window);
        tick(world, delta_time);        
		set_listener_pos(player.location);
		check_shader_hot_reload_queue(&shader_hot_reload_queue, delta_time);

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

        debug_scope { draw_geo_debug(); }
        debug_scope { draw_dev_stats(); }

        update_render_stats();
        
		flush(&entity_render_queue);
        flush_geo_draw();
        flush_text_draw();
         
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
        
		swap_buffers(window);
        PROFILE_FRAME("Game Frame");
        
		clear(frame);

		const s64 end_counter = performance_counter();
		delta_time = (end_counter - begin_counter) / (f32)performance_frequency_s();
		begin_counter = end_counter;

		debug_scope {
			// If dt is too large, we could have resumed from a breakpoint.
			if (delta_time > 1.0f) delta_time = 0.16f;
		}
	}

	stop_hot_reload_thread(&hot_reload_list);
	destroy(window);
    release_core(vm);
    
	return 0;
}
