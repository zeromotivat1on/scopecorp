#pragma once

#define DBGC_UNKNOWN_CMD_WARNING "unknown command: "

#define DBGC_CMD_CLEAR S("clear")
#define DBGC_CMD_LEVEL S("level")

enum Input_Key : u8;
extern Input_Key KEY_SWITCH_CONSOLE;

struct Window_Event;

struct Debug_Console {
    static constexpr u32 MAX_HISTORY_SIZE      = KB(32);
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
    
    bool is_open = false;
};

void console_init();
void console_open();
void console_close();
void console_draw();
void console_add_to_history(String s);
void console_on_input(const Window_Event &event);
void console_on_viewport_resize(s16 width, s16 height);
