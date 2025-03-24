#include "pch.h"
#include "game/game.h"
#include "game/player_control.h"
#include "game/world.h"

#include "log.h"

#include "os/input.h"
#include "os/window.h"

#include "render/text.h"
#include "render/viewport.h"

void handle_window_event(Window *window, Window_Event *event) {
	// @Todo: use input action.
	if (event->type == EVENT_RESIZE) {
        if (window->width != viewport.width || window->height != viewport.height) {
            resize_viewport(&viewport, window->width, window->height);
            
            on_viewport_resize(&world->camera, &viewport);
            world->ed_camera = world->camera;
            
            on_viewport_resize(text_draw_command, &viewport);
        }
	}

	if (event->type == EVENT_KEYBOARD) {
		const s16 key = event->key_code;
		const bool pressed = event->key_pressed;

		if (pressed && key == KEY_ESCAPE)
			close(window);

		if (pressed && key == KEY_F1) {
			if (game_state.mode == MODE_GAME) {
				game_state.mode = MODE_EDITOR;
				lock_cursor(window, true);
			} else {
				game_state.mode = MODE_GAME;
				lock_cursor(window, false);
			}
		}

		if (pressed && key == KEY_F2) {
			if (game_state.camera_behavior == IGNORE_PLAYER)
				game_state.camera_behavior = STICK_TO_PLAYER;
			else if (game_state.camera_behavior == STICK_TO_PLAYER)
				game_state.camera_behavior = FOLLOW_PLAYER;
			else
				game_state.camera_behavior = IGNORE_PLAYER;
		}

		if (pressed && key == KEY_F3) {
			if (game_state.player_movement_behavior == MOVE_INDEPENDENT)
				game_state.player_movement_behavior = MOVE_RELATIVE_TO_CAMERA;
			else
				game_state.player_movement_behavior = MOVE_INDEPENDENT;
		}

		press(key, pressed);
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
