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

#include "render/gfx.h"
#include "render/draw.h"
#include "render/text.h"
#include "render/viewport.h"
#include "render/render_registry.h"

#include "audio/sound.h"

#include "game/world.h"
#include "game/game.h"
#include "editor/hot_reload.h"

static void on_window_event(Window *window, Window_Event *event) {
	handle_event(window, event);
}

int main() {
    PROFILE_START(startup, "Startup");    
    const s64 startup_counter = performance_counter();
    
    void *vm = allocate_core();

	log("Preallocated memory storages: Persistent %.fmb | Frame %.fmb | Temp %.fmb",
		(f32)pers_memory_size / 1024 / 1024, (f32)frame_memory_size / 1024 / 1024, (f32)temp_memory_size / 1024 / 1024);

	init_input_table();

	window = create_window(1280, 720, "Scopecorp", 0, 0);
	if (!window) {
		error("Failed to create window");
		return 1;
	}

	register_event_callback(window, on_window_event);

	init_gfx(window);
	set_gfx_features(GFX_FLAG_BLEND | GFX_FLAG_DEPTH | GFX_FLAG_SCISSOR |
                     GFX_FLAG_CULL_BACK_FACE | GFX_FLAG_WINDING_CCW);
	set_vsync(false);

	init_audio_context();

	init_render_registry(&render_registry);
	load_game_textures(&texture_index_list);
	compile_game_shaders(&shader_index_list);
	create_game_materials(&material_index_list);

	create_game_flip_books(&flip_books);
	load_game_sounds(&sounds);

	Hot_Reload_List hot_reload_list = {0};
	register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);
	start_hot_reload_thread(&hot_reload_list);

	Font *font = create_font(DIR_FONTS "consola.ttf");
	Font_Atlas *atlas = bake_font_atlas(font, 33, 126, 16);
	text_draw_command = create_default_text_draw_command(atlas);

	init_draw_queue(&draw_queue);

	world = push_struct(pers, World);
	init_world(world);

	Player &player = world->player;
	{   // Create player.
        const auto &texture = render_registry.textures[texture_index_list.player_idle[DIRECTION_BACK]];
        const f32 scale_aspect = (f32)texture.width / texture.height;
        const f32 y_scale = 1.0f * scale_aspect;
        const f32 x_scale = y_scale * scale_aspect;

		player.scale = vec3(x_scale, y_scale, 1.0f);
		player.material_index = material_index_list.player;

        // Little uv offset as source textures have small transient border.
        const f32 uv_offset = 0.02f;
		Vertex_PU vertices[4] = { // center in bottom mid point of quad
			{ vec3( 0.5f,  1.0f, 0.0f), vec2(1.0f - uv_offset, 1.0f - uv_offset) },
			{ vec3( 0.5f,  0.0f, 0.0f), vec2(1.0f - uv_offset, 0.0f + uv_offset) },
			{ vec3(-0.5f,  0.0f, 0.0f), vec2(0.0f + uv_offset, 0.0f + uv_offset) },
			{ vec3(-0.5f,  1.0f, 0.0f), vec2(0.0f + uv_offset, 1.0f - uv_offset) },
		};
		Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
		player.vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), (f32 *)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		player.index_buffer_index = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &ground = world->static_meshes[world->static_meshes.add_default()];
	{   // Create ground.
		ground.scale = vec3(16.0f);
		ground.rotation = quat_from_axis_angle(vec3_right, 90.0f);
		ground.material_index = material_index_list.ground;

		set_material_uniform_value(ground.material_index, "u_scale", &ground.scale);

		Vertex_PU vertices[] = {
			{ vec3( 0.5f,  0.5f, 0.0f), vec2(1.0f, 1.0f) },
			{ vec3( 0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3(-0.5f,  0.5f, 0.0f), vec2(0.0f, 1.0f) },
		};
		Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
		ground.vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), (f32 *)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		ground.index_buffer_index = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Static_Mesh &cube = world->static_meshes[world->static_meshes.add_default()];
	{   // Create cube.
		cube.location = vec3(3.0f, 0.5f, 4.0f);
		cube.aabb.min = cube.location - cube.scale * 0.5f;
		cube.aabb.max = cube.aabb.min + cube.scale;

		cube.material_index = material_index_list.cube;

		Vertex_PU vertices[] = {
			// Front face
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

			// Back face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 1.0f) },

			// Left face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

			// Right face
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

			// Top face
			{ vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 1.0f) },

			// Bottom face
			{ vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 0.0f) },
			{ vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 0.0f) },
			{ vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f) },
			{ vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },
		};
		Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
		cube.vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), (f32 *)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[] = {
			// Front face
			0,  1,  2,  1,  3,  2,
			// Back face
			4,  6,  5,  5,  6,  7,
			// Left face
			8,  9, 10,  9, 11, 10,
			// Right face
			12, 14, 13, 13, 14, 15,
			// Bottom face
			16, 17, 18, 17, 19, 18,
			// Top face
			20, 22, 21, 21, 22, 23
		};
		cube.index_buffer_index = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Skybox &skybox = world->skybox;
	{   // Create skybox.
		skybox.material_index = material_index_list.skybox;

		Vertex_PU vertices[] = {
			{ vec3( 1.0f,  1.0f, 0.0f), vec2(1.0f, 1.0f) },
			{ vec3( 1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f) },
			{ vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f) },
			{ vec3(-1.0f,  1.0f, 0.0f), vec2(0.0f, 1.0f) },
		};
		Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
		skybox.vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), (f32 *)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

		u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
		skybox.index_buffer_index = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
	}

	Camera &camera = world->camera;
	camera.mode = MODE_PERSPECTIVE;
	camera.yaw = 90.0f;
	camera.pitch = 0.0f;
	camera.eye = player.location + player.camera_offset;
	camera.at = camera.eye + forward(camera.yaw, camera.pitch);
	camera.up = vec3(0.0f, 1.0f, 0.0f);
	camera.fov = 60.0f;
	camera.near = 0.1f;
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

		clear_screen(vec4(0.9f, 0.4f, 0.5f, 1.0f)); // ugly bright pink
		enqueue_draw_world(&draw_queue, world);

		// @Cleanup: flush before text draw as its overwritten by skybox, fix.
		flush_draw_commands(&draw_queue);

		debug_scope {
            PROFILE_SCOPE("Debug Stats Draw");

			draw_dev_stats(atlas, world);

			const bool has_overlap = overlap(player.aabb, cube.aabb);
            if (has_overlap) {
                player.location -= player.velocity;
            }
            
			char text[256];
			s32 size = stbsp_snprintf(text, sizeof(text), "cube\n\tlocation %s\n\taabb %s %s\noverlap with player %s", to_string(cube.location), to_string(cube.aabb.min), to_string(cube.aabb.max), has_overlap ? "TRUE" : "FALSE");
			vec2 pos = vec2(10.0f, viewport.height * 0.5f);
			draw_text_immediate(text_draw_command, text, size, pos, vec3(1.0f));
		}

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
