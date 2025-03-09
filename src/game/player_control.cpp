#include "pch.h"
#include "player_control.h"
#include "log.h"
#include "flip_book.h"

#include "os/input.h"

#include "game/game.h"
#include "game/world.h"

#include "math/math_core.h"

#include "audio/al.h"
#include "audio/sound.h"

#include "render/texture.h"
#include "render/render_registry.h"

void press(s32 key, bool pressed) {

}

void tick(Player *player, f32 dt) {
	// Force game camera movement/rotation.
	if (input_table.key_states[KEY_SHIFT]) {
		Camera &camera = world->camera;
		const vec3 camera_forward = forward(camera.yaw, camera.pitch);
		const vec3 camera_right = camera.up.cross(camera_forward).normalize();

		const f32 speed = player->ed_camera_speed * dt;
		vec3 velocity;

		if (input_table.key_states[KEY_RIGHT])
			velocity += speed * camera_right;

		if (input_table.key_states[KEY_LEFT])
			velocity -= speed * camera_right;

		if (input_table.key_states[KEY_UP])
			velocity += speed * camera.up;

		if (input_table.key_states[KEY_DOWN])
			velocity -= speed * camera.up;

		camera.eye += velocity.truncate(speed);
		camera.at = camera.eye + camera_forward;
	} else if (input_table.key_states[KEY_CTRL]) {
		Camera &camera = world->camera;
		const f32 speed = 4.0f * player->mouse_sensitivity * dt;

		if (input_table.key_states[KEY_RIGHT])
			camera.yaw -= speed;

		if (input_table.key_states[KEY_LEFT])
			camera.yaw += speed;

		if (input_table.key_states[KEY_UP])
			camera.pitch += speed;

		if (input_table.key_states[KEY_DOWN])
			camera.pitch -= speed;

		camera.pitch = clamp(camera.pitch, -89.0f, 89.0f);
		camera.at = camera.eye + forward(camera.yaw, camera.pitch);
	}

	if (game_state.mode == MODE_GAME) {
		const f32 speed = player->move_speed * dt;
		vec3 velocity;

		// @Todo: use input action instead of direct key state.
		if (game_state.player_movement_behavior == MOVE_INDEPENDENT) {
			if (input_table.key_states[KEY_D]) {
				velocity.x = speed;
				player->move_direction = DIRECTION_RIGHT;
			}

			if (input_table.key_states[KEY_A]) {
				velocity.x = -speed;
				player->move_direction = DIRECTION_LEFT;
			}

			if (input_table.key_states[KEY_W]) {
				velocity.z = speed;
				player->move_direction = DIRECTION_FORWARD;
			}

			if (input_table.key_states[KEY_S]) {
				velocity.z = -speed;
				player->move_direction = DIRECTION_BACK;
			}
		} else if (game_state.player_movement_behavior == MOVE_RELATIVE_TO_CAMERA) {
			Camera &camera = world->camera;
			const vec3 camera_forward = forward(camera.yaw, camera.pitch);
			const vec3 camera_right = camera.up.cross(camera_forward).normalize();

			if (input_table.key_states[KEY_D]) {
				velocity += speed * camera_right;
				player->move_direction = DIRECTION_RIGHT;
			}

			if (input_table.key_states[KEY_A]) {
				velocity -= speed * camera_right;
				player->move_direction = DIRECTION_LEFT;
			}

			if (input_table.key_states[KEY_W]) {
				velocity += speed * camera_forward;
				player->move_direction = DIRECTION_FORWARD;
			}

			if (input_table.key_states[KEY_S]) {
				velocity -= speed * camera_forward;
				player->move_direction = DIRECTION_BACK;
			}
		}

		player->velocity = velocity.truncate(speed);
		player->location += player->velocity;

		if (player->velocity == vec3_zero) {
			// @Cleanup: wrap this!!!
			render_registry.materials[player->draw_data.mti].texture_index = texture_index_list.player_idle[player->move_direction];
		} else {
			player->flip_book = &flip_books.player_move[player->move_direction];
			// @Cleanup: reset old flip book frame time if we've changed to new one.
            tick(player->flip_book, dt);
            // @Cleanup: wrap this!!!
            render_registry.materials[player->draw_data.mti].texture_index = current_frame(player->flip_book);
		}

		if (game_state.camera_behavior == STICK_TO_PLAYER) {
			Camera &camera = world->camera;
			camera.eye = player->location + player->camera_offset;
			camera.at = camera.eye + forward(camera.yaw, camera.pitch);
		} else if (game_state.camera_behavior == FOLLOW_PLAYER) {
			Camera &camera = world->camera;
			const vec3 camera_dead_zone = player->camera_dead_zone;
			const vec3 dead_zone_min = camera.eye - camera_dead_zone * 0.5f;
			const vec3 dead_zone_max = camera.eye + camera_dead_zone * 0.5f;
			const vec3 desired_camera_eye = player->location + player->camera_offset;

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

			camera.eye = lerp(camera.eye, target_eye, player->camera_follow_speed * dt);
			camera.at = camera.eye + forward(camera.yaw, camera.pitch);
		}
	} else if (game_state.mode == MODE_EDITOR) {
		const f32 mouse_sensitivity = player->mouse_sensitivity;
		Camera &camera = world->ed_camera;

		camera.yaw += input_table.mouse_offset_x * mouse_sensitivity * dt;
		camera.pitch += input_table.mouse_offset_y * mouse_sensitivity * dt;
		camera.pitch = clamp(camera.pitch, -89.0f, 89.0f);

		const f32 speed = player->ed_camera_speed * dt;
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

    const vec3 aabb_offset = vec3(player->scale.x * 0.5f, 0.0f, player->scale.x * 0.3f);
	player->aabb.min = player->location - aabb_offset;
	player->aabb.max = player->location + aabb_offset + vec3(0.0f, player->scale.y, 0.0f);

	const Sound &steps_sound = sounds.player_steps;
	if (player->velocity == vec3(0.0f)) {
		s32 state;
		alGetSourcei(steps_sound.source, AL_SOURCE_STATE, &state);
		if (state == AL_PLAYING) alSourceStop(steps_sound.source);
	} else {
		s32 state;
		alGetSourcei(steps_sound.source, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) alSourcePlay(steps_sound.source);
	}
}
