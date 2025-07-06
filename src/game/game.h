#pragma once

enum Game_Mode {
	MODE_GAME,
	MODE_EDITOR,
    MODE_COUNT
};

enum View_Mode_Flag : u32 {
    VIEW_MODE_FLAG_COLLISION = 0x1,
};

enum Camera_Behavior {
    FOLLOW_PLAYER,
	STICK_TO_PLAYER,
    IGNORE_PLAYER,
};

enum Player_Movement_Behavior {
	MOVE_INDEPENDENT,
	MOVE_RELATIVE_TO_CAMERA,
};

enum Property_Change_Type {
    PROPERTY_LOCATION,
    PROPERTY_ROTATION,
    PROPERTY_SCALE,
};

enum Polygon_Mode : u8;

struct Game_State {
	Game_Mode mode = MODE_EDITOR;
    u32 view_mode_flags = 0;
	Camera_Behavior camera_behavior = FOLLOW_PLAYER;
	Player_Movement_Behavior player_movement_behavior = MOVE_RELATIVE_TO_CAMERA;
    Property_Change_Type selected_entity_property_to_change = PROPERTY_LOCATION;
    Polygon_Mode polygon_mode = (Polygon_Mode)0;
};

struct Window_Event;

inline Game_State game_state;

void on_window_resize(s16 width, s16 height);
void on_input_game(Window_Event *event);
void tick_game(f32 dt);

const char *to_string(Game_Mode mode);
const char *to_string(Camera_Behavior behavior);
const char *to_string(Player_Movement_Behavior behavior);
const char *to_string(enum Entity_Type type);
