#pragma once

#include "input.h"

inline const auto CONSOLE_CMD_UNKNOWN_WARNING = S("unknown command: ");

inline const auto CONSOLE_CMD_CLEAR       = S("clear");
inline const auto CONSOLE_CMD_LEVEL       = S("level");
inline const auto CONSOLE_CMD_USAGE_CLEAR = S("usage: clear");
inline const auto CONSOLE_CMD_USAGE_LEVEL = S("usage: level name_with_extension");

inline constexpr auto KEY_OPEN_CONSOLE = KEY_GRAVE_ACCENT;

struct Window_Event;

struct Console {
    static constexpr u32 MAX_MESSAGE_COUNT     = 4096;
    static constexpr u32 MAX_HISTORY_SIZE      = Megabytes(1);
    static constexpr u32 MAX_INPUT_SIZE        = 64;
    static constexpr f32 CURSOR_BLINK_INTERVAL = 0.5f;    
    static constexpr f32 MARGIN                = 64.0f;
    static constexpr f32 PADDING               = 16.0f;
    
    String    history;
    String    input;
    String    messages[MAX_MESSAGE_COUNT];
    Log_Level log_levels[MAX_MESSAGE_COUNT];
    s32       message_count = 0;
    f32       cursor_blink_dt = 0.0f;
    bool      opened = false;
    s32       scroll_pos = 0;
};

Console *get_console            ();
void     open_console           ();
void     close_console          ();
void     update_console         ();
void     add_to_console_history (String s);
void     add_to_console_history (Log_Level level, String s);
void     on_console_input       (const Window_Event *e);
