#pragma once

#define DEBUG_CONSOLE_UNKNOWN_COMMAND_WARNING "unknown command: "

#define DEBUG_CONSOLE_COMMAND_CLEAR "clear"

inline constexpr s32 MAX_DEBUG_CONSOLE_HISTORY_SIZE = KB(32);
inline constexpr s32 MAX_DEBUG_CONSOLE_INPUT_SIZE   = 64;

inline constexpr f32 DEBUG_CONSOLE_MARGIN  = 100.0f;
inline constexpr f32 DEBUG_CONSOLE_PADDING = 16.0f;

inline constexpr f32 DEBUG_CONSOLE_CURSOR_BLINK_INTERVAL = 0.5f;

struct Debug_Console {
    char *history = null;
    s32 history_size = 0;
    f32 history_height = 0.0f;
    f32 history_max_width = 0.0f;
    f32 history_y = 0.0f;
    f32 history_min_y = 0.0f;
    
    char input[MAX_DEBUG_CONSOLE_INPUT_SIZE];
    s32 input_size = 0;

    f32 cursor_blink_dt = 0.0f;
    
    bool is_open = false;
};

inline Debug_Console debug_console;

void init_debug_console();
void open_debug_console();
void close_debug_console();
void draw_debug_console();
void add_to_debug_console_history(const char *text, u32 count);
void on_debug_console_input(u32 character);
void on_debug_console_scroll(s32 delta);
void on_debug_console_resize(s16 width, s16 height);
