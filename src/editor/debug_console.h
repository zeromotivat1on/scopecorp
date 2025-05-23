#pragma once

inline constexpr s32 MAX_DEBUG_CONSOLE_BUFFER_SIZE     = KB(32);
inline constexpr s32 MAX_DEBUG_CONSOLE_TEXT_INPUT_SIZE = 256;

struct Debug_Console {
    char *buffer = null;
    char *text_to_draw = null;
    s32 buffer_size = 0;
    s32 draw_count  = 0;
    bool is_open = false;
    
    char text_input[MAX_DEBUG_CONSOLE_TEXT_INPUT_SIZE];
    s32 text_input_size = 0;
};

inline Debug_Console debug_console;

void init_debug_console();
void open_debug_console();
void close_debug_console();
void draw_debug_console();
void on_debug_console_text_input(u32 character);
