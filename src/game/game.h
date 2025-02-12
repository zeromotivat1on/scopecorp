#pragma once

enum Game_Mode
{
    MODE_GAME,
    MODE_EDITOR
};

enum Camera_Behavior
{
    IGNORE_PLAYER,
    STICK_TO_PLAYER,
};

enum Player_Movement_Behavior
{
    INDEPENDENT,
    RELATIVE_TO_CAMERA,
};

struct Game_State
{
    Game_Mode mode = MODE_GAME;
    Camera_Behavior camera_behavior = IGNORE_PLAYER;
    Player_Movement_Behavior player_movement_behavior = INDEPENDENT;
};

inline Game_State game_state;

void handle_event(struct Window* window, struct Window_Event* event);

const char* to_string(Game_Mode mode);
const char* to_string(Camera_Behavior behavior);
const char* to_string(Player_Movement_Behavior behavior);
