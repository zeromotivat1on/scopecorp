#pragma once

#include "string_builder.h"
#include "command_buffer.h"

struct Game_Logger_Data {
    String_Builder messages;
};

inline Game_Logger_Data game_logger_data;

void game_logger_proc  (String message, String ident, Log_Level level, void *logger_data);
void flush_game_logger ();

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

struct Game_State {
    u32 view_mode_flags = 0;
	Camera_Behavior camera_behavior = STICK_TO_PLAYER;
	Player_Movement_Behavior player_movement_behavior = MOVE_INDEPENDENT;
    Property_Change_Type selected_entity_property_to_change = PROPERTY_LOCATION;
    Polygon_Mode polygon_mode = POLYGON_FILL;
};

inline Game_State game_state;

struct Window_Event;

void on_window_resize(u16 width, u16 height);

void init_asset_storages ();
void load_game_assets    ();

void simulate_game ();
void on_game_input (const Window_Event *e);
void on_game_push  ();
void on_game_pop   ();

const char *to_string(Program_Mode mode);
const char *to_string(Camera_Behavior behavior);
const char *to_string(Player_Movement_Behavior behavior);
