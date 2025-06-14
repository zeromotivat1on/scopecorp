#pragma once

extern s16 KEY_CLOSE_WINDOW;
extern s16 KEY_SWITCH_EDITOR_MODE;
extern s16 KEY_SWITCH_COLLISION_VIEW;

struct Window_Event;

void on_input_editor(Window_Event *event);
void tick_editor(f32 dt);
