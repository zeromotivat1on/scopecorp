#pragma once

inline constexpr f32 EDITOR_REPORT_SHOW_TIME = 2.0f;
inline constexpr f32 EDITOR_REPORT_FADE_TIME = 0.5f;

struct Entity;
struct Window_Event;
struct Level_Set;

struct Editor {
    Entity *mouse_picked_entity = null;
    String  report_string;
    f32     report_time = 0.0f;
    f32     camera_speed = 4.0f;
};

inline Editor editor;

void init_level_editor_hub ();
void update_editor         ();
void on_push_editor        ();
void on_pop_editor         ();
void on_event_editor       (const Window_Event *e);
void screen_report         (const char *cs, ...);
