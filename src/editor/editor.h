#pragma once

enum Input_Key : u8;

extern Input_Key KEY_CLOSE_WINDOW;
extern Input_Key KEY_SWITCH_EDITOR_MODE;
extern Input_Key KEY_SWITCH_POLYGON_MODE;
extern Input_Key KEY_SWITCH_COLLISION_VIEW;

struct Game_World;
struct Entity;
struct Window_Event;

struct Game_Editor {
    Entity *mouse_picked_entity = null;
};

inline Game_Editor Editor;

void on_input_editor(const Window_Event *event);
void tick_editor(f32 dt);
void editor_report(const char *str, ...);

void mouse_pick_entity(Game_World *world, Entity *e);
void mouse_unpick_entity(Game_World *world);
