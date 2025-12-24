#pragma once

#include "input.h"

#define CONSOLE_UNKNOWN_CMD_WARNING "unknown command: "

#define CONSOLE_CMD_CLEAR S("clear")
#define CONSOLE_CMD_LEVEL S("level")

inline constexpr auto KEY_OPEN_CONSOLE = KEY_GRAVE_ACCENT;

struct Window_Event;

struct Console {
    static constexpr u32 MAX_HISTORY_SIZE      = Megabytes(1);
    static constexpr u32 MAX_INPUT_SIZE        = 64;
    static constexpr f32 CURSOR_BLINK_INTERVAL = 0.5f;    
    static constexpr f32 MARGIN                = 100.0f;
    static constexpr f32 PADDING               = 16.0f;
    
    f32 history_height = 0.0f;
    f32 history_max_width = 0.0f;
    f32 history_y = 0.0f;
    f32 history_min_y = 0.0f;

    String history;
    String input;
    
    f32 cursor_blink_dt = 0.0f;
    
    bool opened = false;
};

Console *get_console ();

void init_console               ();
void open_console               ();
void close_console              ();
void update_console             ();
void add_to_console_history     (String s);
void on_console_input           (const Window_Event *e);
void on_console_viewport_resize (s16 width, s16 height);
