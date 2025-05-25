#pragma once

#define DEBUG_CONSOLE_UNKNOWN_COMMAND_WARNING "unknown command: "

#define DEBUG_CONSOLE_COMMAND_CLEAR "clear"

inline constexpr s32 MAX_DEBUG_CONSOLE_HISTORY_SIZE = KB(32);
inline constexpr s32 MAX_DEBUG_CONSOLE_INPUT_SIZE   = 64;

struct Debug_Console {
    char *history = null;
    s32 history_size = 0;
    bool is_open = false;
    
    char input[MAX_DEBUG_CONSOLE_INPUT_SIZE];
    s32 input_size = 0;
};

inline Debug_Console debug_console;

void init_debug_console();
void open_debug_console();
void close_debug_console();
void draw_debug_console();
void on_debug_console_input(u32 character);
