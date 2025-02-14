#pragma once

enum Game_Mode
{
    MODE_GAME,
    MODE_EDITOR
};

enum Camera_Behavior
{
    IGNORE_PLAYER,   // static position
    STICK_TO_PLAYER, // immediate position update
    FOLLOW_PLAYER,   // smooth following
};

enum Player_Movement_Behavior
{
    MOVE_INDEPENDENT,
    MOVE_RELATIVE_TO_CAMERA,
};

struct Game_State
{
    Game_Mode mode = MODE_GAME;
    Camera_Behavior camera_behavior = FOLLOW_PLAYER;
    Player_Movement_Behavior player_movement_behavior = MOVE_RELATIVE_TO_CAMERA;
};

inline Game_State game_state;

void handle_event(struct Window* window, struct Window_Event* event);

const char* to_string(Game_Mode mode);
const char* to_string(Camera_Behavior behavior);
const char* to_string(Player_Movement_Behavior behavior);
