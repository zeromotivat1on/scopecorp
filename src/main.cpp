#include "pch.h"
#include "log.h"
#include "font.h"
#include "flip_book.h"
#include "memory_storage.h"
#include "profile.h"
#include "stb_sprintf.h"

#include "os/thread.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"

#include "render/draw.h"
#include "render/debug_draw.h"
#include "render/text.h"
#include "render/viewport.h"
#include "render/render_registry.h"

#include "audio/sound.h"

#include "game/world.h"
#include "game/game.h"
#include "editor/hot_reload.h"

extern s32 draw_call_count;

int main() {
    PROFILE_START(startup, "Startup");    
    const s64 startup_counter = performance_counter();
    
    void *vm = allocate_core();
    if (!vm) return 1;
    
	log("Preallocated memory storages: Persistent %.fmb | Frame %.fmb | Temp %.fmb",
		(f32)pers_memory_size / 1024 / 1024, (f32)frame_memory_size / 1024 / 1024, (f32)temp_memory_size / 1024 / 1024);

	window = create_window(1280, 720, "Scopecorp", 0, 0);
	if (!window) {
		error("Failed to create window");
		return 1;
	}

	register_event_callback(window, handle_window_event);
    
	init_input_table();

	init_draw(window);
	set_vsync(false);

	init_render_registry(&render_registry);

    const Texture_Format_Type color_attachment_formats[] = { TEXTURE_FORMAT_RGB_8, TEXTURE_FORMAT_RED_INTEGER };
    const Texture_Format_Type depth_attachment_format = TEXTURE_FORMAT_DEPTH_24_STENCIL_8;
    viewport.frame_buffer_index = create_frame_buffer(window->width, window->height, color_attachment_formats, c_array_count(color_attachment_formats), depth_attachment_format);

	load_game_textures(&texture_index_list);
	compile_game_shaders(&shader_index_list);
	create_game_materials(&material_index_list);
	create_game_flip_books(&flip_books);

    init_audio_context();
	load_game_sounds(&sounds);

	Hot_Reload_List hot_reload_list = {0};
	register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);
	start_hot_reload_thread(&hot_reload_list);

	Font *font = create_font(DIR_FONTS "consola.ttf");
	Font_Atlas *atlas = bake_font_atlas(font, 33, 126, 16);
	text_draw_command = create_default_text_draw_command(atlas);
    
	world = push_struct(pers, World);
	init_world(world);

    init_draw_queue(&world_draw_queue, MAX_DRAW_QUEUE_SIZE);
    init_debug_geometry_draw_queue();

	Player &player = world->player;
	{   // Create player.
        player.id = 1;
        
        const auto &texture = render_registry.textures[texture_index_list.player_idle[DIRECTION_BACK]];
        const f32 scale_aspect = (f32)texture.width / texture.height;
        const f32 y_scale = 1.0f * scale_aspect;
        const f32 x_scale = y_scale * scale_aspect;

        player.aabb_index = world->aabbs.add_default();
        
		player.scale = vec3(x_scale, y_scale, 1.0f);
        player.location = vec3(0.0f, F32_MIN, 0.0f);

		player.draw_data.mti = material_index_list.player;

        static const vec3 uv_scale = vec3(1.0f);
		set_material_uniform_value(player.draw_data.mti, "u_uv_scale", &uv_scale);
        
        // Little uv offset as source textures have small transient border.
        const f32 uv_offset = 0.02f;
		Vertex_Entity vertices[4] = { // center in bottom mid point of quad
			{ vec3( 0.5f,  1.0f, 0.0f), vec2(1.0f - uv_offset, 1.0f - uv_offset), player.id },
			{ vec3( 0.5f,  0.0f, 0.0f), vec2(1.0f - uv_offset, 0.0f + uv_offset), player.id },
			{ vec3(-0.5f,  0.0f, 0.0f), vec2(0.0f + uv_offset, 0.0f + uv_offset), player.id },
			{ vec3(-0.5f,  1.0f, 0.0f), vec2(0.0f + uv_offset, 1.0f - uv_offset), player.id },
		};
		Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_2, VERTEX_S32 };
		player.draw_data.vbi = create_vertex_buffer(components, c_array_count(components), (f32 *)vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		player.draw_data.ibi = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &ground = world->static_meshes[create_static_mesh(world)];
	{   // Create ground.
        ground.id = 10;
                
		ground.scale = vec3(16.0f, 16.0f, 0.0f);
        ground.rotation = quat_from_axis_angle(vec3_right, 90.0f);

        auto &aabb = world->aabbs[ground.aabb_index];
        const vec3 aabb_offset = vec3(16.0f, 0.0f, 16.0f);
        aabb.min = ground.location - aabb_offset * 0.5f;
		aabb.max = aabb.min + aabb_offset;

        ground.draw_data.mti = material_index_list.ground;

        static const vec3 uv_scale = vec3(16.0f);
		set_material_uniform_value(ground.draw_data.mti, "u_uv_scale", &uv_scale);

		Vertex_Entity vertices[] = {
			{ vec3( 0.5f,  0.5f, 0.0f), vec2(1.0f, 1.0f), ground.id },
			{ vec3( 0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f), ground.id },
			{ vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 0.0f), ground.id },
			{ vec3(-0.5f,  0.5f, 0.0f), vec2(0.0f, 1.0f), ground.id },
		};
		Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_2, VERTEX_S32 };
		ground.draw_data.vbi = create_vertex_buffer(components, c_array_count(components), (f32 *)vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		ground.draw_data.ibi = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &cube = world->static_meshes[create_static_mesh(world)];
	{   // Create cube.
        cube.id = 20;
                
		cube.location = vec3(3.0f, 0.5f, 4.0f);

        auto &aabb = world->aabbs[cube.aabb_index];
		aabb.min = cube.location - cube.scale * 0.5f;
		aabb.max = aabb.min + cube.scale;

		cube.draw_data.mti = material_index_list.cube;

        static const vec3 uv_scale = vec3(1.0f);
		set_material_uniform_value(cube.draw_data.mti, "u_uv_scale", &uv_scale);
        
		Vertex_Entity vertices[] = {
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
		Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_2, VERTEX_S32 };
		cube.draw_data.vbi = create_vertex_buffer(components, c_array_count(components), (f32 *)vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[] = {
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
		cube.draw_data.ibi = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Skybox &skybox = world->skybox;
	{   // Create skybox.
        skybox.id = 999;

		skybox.draw_data.mti = material_index_list.skybox;

		Vertex_PU vertices[] = {
			{ vec3( 1.0f,  1.0f, 1.0f - F32_EPSILON), vec2(1.0f, 1.0f) },
			{ vec3( 1.0f, -1.0f, 1.0f - F32_EPSILON), vec2(1.0f, 0.0f) },
			{ vec3(-1.0f, -1.0f, 1.0f - F32_EPSILON), vec2(0.0f, 0.0f) },
			{ vec3(-1.0f,  1.0f, 1.0f - F32_EPSILON), vec2(0.0f, 1.0f) },
		};
		Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_2 };
		skybox.draw_data.vbi = create_vertex_buffer(components, c_array_count(components), (f32 *)vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		skybox.draw_data.ibi = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
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

	f32 dt = 0.0f;
	s64 begin_counter = performance_counter();
	const f32 frequency = (f32)performance_frequency();

    log("Startup took %.2fs", (performance_counter() - startup_counter) / (f32)performance_frequency());
    PROFILE_END(startup);
    
	while (alive(window)) {
        PROFILE_SCOPE("Game Frame");
 
		poll_events(window);
        
        tick(world, dt);
        
		set_listener_pos(player.location);
		check_shader_hot_reload_queue(&shader_hot_reload_queue, dt);

        Draw_Command framebuffer_command;
        framebuffer_command.frame_buffer_index = viewport.frame_buffer_index;
        draw(&framebuffer_command);
        
		clear_screen(vec3_white, CLEAR_FLAG_COLOR | CLEAR_FLAG_DEPTH | CLEAR_FLAG_STENCIL);
		draw_world(world);

        const vec3 player_center_location = player.location + vec3(0.0f, player.scale.y * 0.5f, 0.0f);
        draw_debug_line(player_center_location, player_center_location + normalize(player.velocity) * 0.5f, vec3_red);

        if (player.collide_aabb_index != INVALID_INDEX) {
            draw_debug_aabb(world->aabbs[player.aabb_index],         vec3_green);
            draw_debug_aabb(world->aabbs[player.collide_aabb_index], vec3_green);
        }
        
        // Send draw call count to dev stats.
        draw_call_count = world_draw_queue.count;

		// @Cleanup: flush before text draw as its overwritten by skybox, fix.        
		flush(&world_draw_queue);
        flush(&debug_draw_queue);
        
        debug_scope {
            PROFILE_SCOPE("Debug Stats Draw");
			draw_dev_stats(atlas, world);
		}

        framebuffer_command.flags = DRAW_FLAG_RESET;
        draw(&framebuffer_command);

        clear_screen(vec3_red, CLEAR_FLAG_COLOR);
        draw_frame_buffer(viewport.frame_buffer_index, 0);
        
		swap_buffers(window);
        PROFILE_FRAME("Game Frame");
        
		clear(frame);

		const s64 end_counter = performance_counter();
		dt = (end_counter - begin_counter) / frequency;
		begin_counter = end_counter;

		debug_scope {
			// If dt is too large, we could have resumed from a breakpoint.
			if (dt > 1.0f) dt = 0.16f;
		}
	}

	stop_hot_reload_thread(&hot_reload_list);
	destroy(window);
    release_core(vm);
    
	return 0;
}
