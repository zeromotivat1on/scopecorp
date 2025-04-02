#include "pch.h"
#include "game/game.h"
#include "game/player_control.h"
#include "game/world.h"

#include "log.h"
#include "profile.h"
#include "flip_book.h"

#include "math/math_core.h"

#include "os/input.h"
#include "os/window.h"

#include "render/text.h"
#include "render/viewport.h"
#include "render/render_registry.h"

#include "audio/al.h"
#include "audio/sound.h"

void on_window_event(Window *window, Window_Event *event) {
	// @Todo: use input action.
	if (event->type == EVENT_RESIZE) {
        resize_viewport(&viewport, window->width, window->height);
        viewport.orthographic_projection = mat4_orthographic(0, viewport.width, 0, viewport.height, -1, 1);
            
        on_viewport_resize(&world->camera, &viewport);
        world->ed_camera = world->camera;

        set_material_uniform_value(text_draw_buffer.material_index, "u_projection", &viewport.orthographic_projection);

        const auto &frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];
        const vec2 resolution = vec2(frame_buffer.width, frame_buffer.height);
        set_material_uniform_value(frame_buffer.material_index, "u_resolution", &resolution);
	}

	if (event->type == EVENT_KEYBOARD) {
		press(event->key_code, event->key_pressed);
	}

	if (event->type == EVENT_TEXT_INPUT) {
        
	}

	if (event->type == EVENT_MOUSE) {
        click(event->key_code, event->key_pressed);
	}

	if (event->type == EVENT_QUIT) {
		log("EVENT_QUIT");
	}
}

void press(s32 key, bool pressed) {
    const bool ctrl_down = input_table.key_states[KEY_CTRL];
        
    if (pressed && key == KEY_ESCAPE)
        close(window);

    if (pressed && ctrl_down && key == KEY_G) {
        game_state.mode = MODE_GAME;
        lock_cursor(window, true);
    } else if (pressed && ctrl_down && key == KEY_E) {
        game_state.mode = MODE_EDITOR;
        lock_cursor(window, false);
    }

    if (game_state.mode == MODE_EDITOR) {
        if (pressed && key == KEY_F1) {
            lock_cursor(window, !window->cursor_locked);
        }
    } else if (game_state.mode == MODE_GAME) {
        if (pressed && key == KEY_F1) {
            game_state.camera_behavior = (Camera_Behavior)(((s32)game_state.camera_behavior + 1) % 3);
        } else if (pressed && key == KEY_F2) {
            game_state.player_movement_behavior = (Player_Movement_Behavior)(((s32)game_state.player_movement_behavior + 1) % 2);
        }
    }
    
    if (pressed && ctrl_down && key == KEY_R) {
        world->player.location = vec3(0.0f, F32_MIN, 0.0f);
        world->camera.eye = world->player.location + world->player.camera_offset;
        world->camera.at = world->camera.eye + forward(world->camera.yaw, world->camera.pitch);
    }
}

void click(s32 key, bool pressed) {
    if (game_state.mode == MODE_EDITOR) {
        if (key == MOUSE_RIGHT) {
            lock_cursor(window, !window->cursor_locked);
        }
        
        if (pressed && key == MOUSE_LEFT) {
            world->selected_entity_id = read_frame_buffer_pixel(viewport.frame_buffer_index, 1, input_table.mouse_x, input_table.mouse_y);
        } else if (pressed && key == MOUSE_RIGHT) {
            world->selected_entity_id = INVALID_ENTITY_ID;
        }
    }
}

void init_world(World *world) {
	*world = World();

	world->static_meshes = Sparse_Array<Static_Mesh>(MAX_STATIC_MESHES);
	world->aabbs         = Sparse_Array<AABB>(MAX_AABBS);
}

static void try_outline_mouse_picked_entity(Entity *e) {
    if (e->id == world->selected_entity_id) {
        e->flags |= ENTITY_FLAG_SELECTED_IN_EDITOR;
    } else {
        e->flags &= ~ENTITY_FLAG_SELECTED_IN_EDITOR;
    }
}

void tick(World *world, f32 dt) {
    PROFILE_SCOPE("Tick World");
    
	world->dt = dt;

	Camera *camera = desired_camera(world);
	world->camera_view = camera_view(camera);
	world->camera_proj = camera_projection(camera);
    world->camera_view_proj = world->camera_view * world->camera_proj;
    
	auto &skybox = world->skybox;
    try_outline_mouse_picked_entity(&skybox);
	skybox.uv_offset = camera->eye;
	set_material_uniform_value(skybox.draw_data.material_index, "u_scale", &skybox.uv_scale);
	set_material_uniform_value(skybox.draw_data.material_index, "u_offset", &skybox.uv_offset);

	for (s32 i = 0; i < world->static_meshes.count; ++i) {
		auto *mesh = world->static_meshes.items + i;
        try_outline_mouse_picked_entity(mesh);
		mesh->mvp = mat4_transform(mesh->location, mesh->rotation, mesh->scale) * world->camera_view_proj;
		set_material_uniform_value(mesh->draw_data.material_index, "u_transform", mesh->mvp.ptr());
	}

	tick_player(world);

    auto &player = world->player;
    try_outline_mouse_picked_entity(&player);
	player.mvp = mat4_transform(player.location, player.rotation, player.scale) * world->camera_view_proj;
	set_material_uniform_value(player.draw_data.material_index, "u_transform", player.mvp.ptr());

	if (game_state.mode == MODE_GAME) {
		// Editor camera should follow game one during gameplay.
		world->ed_camera = world->camera;
	}
}

Camera *desired_camera(World *world) {
	if (game_state.mode == MODE_GAME)   return &world->camera;
	if (game_state.mode == MODE_EDITOR) return &world->ed_camera;

	error("Failed to get desired camera from world in unknown game mode %d", game_state.mode);
	return null;
}

s32 create_static_mesh(World *world) {
    const s32 index = world->static_meshes.add_default();
    world->static_meshes[index].aabb_index = world->aabbs.add_default();
    return index;
}

void tick_player(World* world) {
    const f32 dt = world->dt;
    auto &player = world->player;

    const auto last_move_direction = player.move_direction;
    
	if (game_state.mode == MODE_GAME) {
		const f32 speed = player.move_speed * dt;
		vec3 velocity;

		// @Todo: use input action instead of direct key state.
		if (game_state.player_movement_behavior == MOVE_INDEPENDENT) {
			if (input_table.key_states[KEY_D]) {
				velocity.x = speed;
				player.move_direction = DIRECTION_RIGHT;
			}

			if (input_table.key_states[KEY_A]) {
				velocity.x = -speed;
				player.move_direction = DIRECTION_LEFT;
			}

			if (input_table.key_states[KEY_W]) {
				velocity.z = speed;
				player.move_direction = DIRECTION_FORWARD;
			}

			if (input_table.key_states[KEY_S]) {
				velocity.z = -speed;
				player.move_direction = DIRECTION_BACK;
			}
		} else if (game_state.player_movement_behavior == MOVE_RELATIVE_TO_CAMERA) {
			Camera &camera = world->camera;
			const vec3 camera_forward = forward(camera.yaw, camera.pitch);
			const vec3 camera_right = camera.up.cross(camera_forward).normalize();

			if (input_table.key_states[KEY_D]) {
				velocity += speed * camera_right;
				player.move_direction = DIRECTION_RIGHT;
			}

			if (input_table.key_states[KEY_A]) {
				velocity -= speed * camera_right;
				player.move_direction = DIRECTION_LEFT;
			}

			if (input_table.key_states[KEY_W]) {
				velocity += speed * camera_forward;
				player.move_direction = DIRECTION_FORWARD;
			}

			if (input_table.key_states[KEY_S]) {
				velocity -= speed * camera_forward;
				player.move_direction = DIRECTION_BACK;
			}
		}

		player.velocity = velocity.truncate(speed);

		if (game_state.camera_behavior == STICK_TO_PLAYER) {
			Camera &camera = world->camera;
			camera.eye = player.location + player.camera_offset;
			camera.at = camera.eye + forward(camera.yaw, camera.pitch);
		} else if (game_state.camera_behavior == FOLLOW_PLAYER) {
			Camera &camera = world->camera;
			const vec3 camera_dead_zone = player.camera_dead_zone;
			const vec3 dead_zone_min = camera.eye - camera_dead_zone * 0.5f;
			const vec3 dead_zone_max = camera.eye + camera_dead_zone * 0.5f;
			const vec3 desired_camera_eye = player.location + player.camera_offset;

			vec3 target_eye;
			if (desired_camera_eye.x < dead_zone_min.x)
				target_eye.x = desired_camera_eye.x + camera_dead_zone.x * 0.5f;
			else if (desired_camera_eye.x > dead_zone_max.x)
				target_eye.x = desired_camera_eye.x - camera_dead_zone.x * 0.5f;
			else
				target_eye.x = camera.eye.x;

			if (desired_camera_eye.y < dead_zone_min.y)
				target_eye.y = desired_camera_eye.y + camera_dead_zone.y * 0.5f;
			else if (desired_camera_eye.y > dead_zone_max.y)
				target_eye.y = desired_camera_eye.y - camera_dead_zone.y * 0.5f;
			else
				target_eye.y = camera.eye.y;

			if (desired_camera_eye.z < dead_zone_min.z)
				target_eye.z = desired_camera_eye.z + camera_dead_zone.z * 0.5f;
			else if (desired_camera_eye.z > dead_zone_max.z)
				target_eye.z = desired_camera_eye.z - camera_dead_zone.z * 0.5f;
			else
				target_eye.z = camera.eye.z;

			camera.eye = lerp(camera.eye, target_eye, player.camera_follow_speed * dt);
			camera.at = camera.eye + forward(camera.yaw, camera.pitch);
		}
	} else if (game_state.mode == MODE_EDITOR) {
        const f32 mouse_sensitivity = player.mouse_sensitivity;
        Camera &camera = world->ed_camera;

        if (window->cursor_locked) {   
            camera.yaw += input_table.mouse_offset_x * mouse_sensitivity * dt;
            camera.pitch += input_table.mouse_offset_y * mouse_sensitivity * dt;
            camera.pitch = clamp(camera.pitch, -89.0f, 89.0f);
        }
                    
        const f32 speed = player.ed_camera_speed * dt;
        const vec3 camera_forward = forward(camera.yaw, camera.pitch);
        const vec3 camera_right = camera.up.cross(camera_forward).normalize();

        vec3 velocity;

        if (input_table.key_states[KEY_D])
            velocity += speed * camera_right;

        if (input_table.key_states[KEY_A])
            velocity -= speed * camera_right;

        if (input_table.key_states[KEY_W])
            velocity += speed * camera_forward;

        if (input_table.key_states[KEY_S])
            velocity -= speed * camera_forward;

        if (input_table.key_states[KEY_E])
            velocity += speed * camera.up;

        if (input_table.key_states[KEY_Q])
            velocity -= speed * camera.up;

        camera.eye += velocity.truncate(speed);
        camera.at = camera.eye + camera_forward;
	}

    player.collide_aabb_index = INVALID_INDEX;
    auto &player_aabb = world->aabbs[player.aabb_index];
    
    for (s32 i = 0; i < world->aabbs.count; ++i) {
        if (i == player.aabb_index) continue;
        
        const auto &aabb = world->aabbs[i];
        const vec3 resolved_velocity = resolve_moving_static(player_aabb, aabb, player.velocity);
        if (resolved_velocity != player.velocity) {
            player.velocity = resolved_velocity;
            player.collide_aabb_index = i;
            break;
        }
    }

    if (player.velocity == vec3_zero) {
        render_registry.materials[player.draw_data.material_index].texture_index = texture_index_list.player_idle[player.move_direction];
    } else {
        player.flip_book = &flip_books.player_move[player.move_direction];
        tick(player.flip_book, dt);
        render_registry.materials[player.draw_data.material_index].texture_index = current_frame(player.flip_book);
    }
            
    player.location += player.velocity;
    
    const vec3 aabb_offset = vec3(player.scale.x * 0.5f, 0.0f, player.scale.x * 0.3f);
	player_aabb.min = player.location - aabb_offset;
	player_aabb.max = player.location + aabb_offset + vec3(0.0f, player.scale.y, 0.0f);
            
	const Sound &steps_sound = sounds.player_steps;
	if (player.velocity == vec3_zero) {
		s32 state;
		alGetSourcei(steps_sound.source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING) alSourceStop(steps_sound.source);
	} else {
		s32 state;
		alGetSourcei(steps_sound.source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) alSourcePlay(steps_sound.source);
	}
}

const char *to_string(Game_Mode mode) {
	switch (mode) {
	case MODE_GAME:   return "GAME";
	case MODE_EDITOR: return "EDITOR";
	default:          return "UNKNOWN";
	}
}

const char *to_string(Camera_Behavior behavior) {
	switch (behavior) {
	case IGNORE_PLAYER:   return "IGNORE_PLAYER";
	case STICK_TO_PLAYER: return "STICK_TO_PLAYER";
	case FOLLOW_PLAYER:   return "FOLLOW_PLAYER";
	default:              return "UNKNOWN";
	}
}

const char *to_string(Player_Movement_Behavior behavior) {
	switch (behavior) {
	case MOVE_INDEPENDENT:        return "MOVE_INDEPENDENT";
	case MOVE_RELATIVE_TO_CAMERA: return "MOVE_RELATIVE_TO_CAMERA";
	default:                      return "UNKNOWN";
	}
}
