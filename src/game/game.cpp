#include "pch.h"
#include "game.h"
#include "input.h"
#include "window.h"
#include "player_control.h"

void handle_event(Window* window, Window_Event* event)
{
    // @Todo: use input action.

    if (event->type == EVENT_KEYBOARD)
    {
        const s16 key = event->key_code;
        const bool pressed = event->key_pressed;
        
        if (pressed && key == KEY_ESCAPE)
            close(window);

        if (pressed && key == KEY_F9)
        {
            if (game_state.mode == MODE_GAME)
            {
                game_state.mode = MODE_EDITOR;
                lock_cursor(window, true);
            }
            else
            {
                game_state.mode = MODE_GAME;
                lock_cursor(window, false);
            }
        }

        if (pressed && key == KEY_1)
            game_state.camera_behavior = IGNORE_PLAYER;
        else if (pressed && key == KEY_2)
            game_state.camera_behavior = STICK_TO_PLAYER;

        if (pressed && key == KEY_3)
            game_state.player_movement_behavior = INDEPENDENT;
        else if (pressed && key == KEY_4)
            game_state.player_movement_behavior = RELATIVE_TO_CAMERA;
        
        press(key, pressed);
    }
    
    if (event->type == EVENT_TEXT_INPUT)
    {
        
    }
    
    if (event->type == EVENT_MOUSE)
    {
        
    }
    
    if (event->type == EVENT_QUIT)
    {
        log("EVENT_QUIT");
    }
}

const char* to_string(Game_Mode mode)
{
    switch (mode)
    {
    case MODE_GAME: return "GAME";
    case MODE_EDITOR: return "EDITOR";
    default: return "UNKNOWN";
    }
}

const char* to_string(Camera_Behavior behavior)
{
    switch (behavior)
    {
    case IGNORE_PLAYER: return "IGNORE_PLAYER";
    case STICK_TO_PLAYER: return "STICK_TO_PLAYER";
    default: return "UNKNOWN";
    }
}

const char* to_string(Player_Movement_Behavior behavior)
{
    switch (behavior)
    {
    case INDEPENDENT: return "INDEPENDENT";
    case RELATIVE_TO_CAMERA: return "RELATIVE_TO_CAMERA";
    default: return "UNKNOWN";
    }
}
