#pragma once

extern s16 KEY_CLOSE_WINDOW;
extern s16 KEY_SWITCH_EDITOR_MODE;
extern s16 KEY_SWITCH_COLLISION_VIEW;

struct World;
struct Entity;
struct Window_Event;

void on_input_editor(Window_Event *event);
void tick_editor(f32 dt);
void screen_report(const char *str, ...);

void mouse_pick_entity(World *world, Entity *e);
void mouse_unpick_entity(World *world);
