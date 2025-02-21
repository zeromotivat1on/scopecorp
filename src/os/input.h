#pragma once

enum Input_Key : s16
{
    KEY_UNKNOWN,

    // Printable keys.
    KEY_SPACE,
    KEY_APOSTROPHE,
    KEY_COMMA,
    KEY_MINUS,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_SEMICOLON,
    KEY_EQUAL,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_LEFT_BRACKET,
    KEY_BACKSLASH,
    KEY_RIGHT_BRACKET,
    KEY_GRAVE_ACCENT,

    // Function keys.
    KEY_ESCAPE,
    KEY_ENTER,
    KEY_TAB,
    KEY_BACKSPACE,
    KEY_INSERT,
    KEY_DELETE,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_DOWN,
    KEY_UP,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_CAPS_LOCK,
    KEY_SCROLL_LOCK,
    KEY_NUM_LOCK,
    KEY_PRINT_SCREEN,
    KEY_PAUSE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_F25,
    KEY_KP_0,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,
    KEY_KP_DECIMAL,
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT,
    KEY_KP_ADD,
    KEY_KP_ENTER,
    KEY_KP_EQUAL,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_ALT,
    KEY_LEFT_SHIFT,
    KEY_LEFT_CTRL,
    KEY_LEFT_ALT,
    KEY_LEFT_SUPER,
    KEY_RIGHT_SHIFT,
    KEY_RIGHT_CTRL,
    KEY_RIGHT_ALT,
    KEY_RIGHT_SUPER,
    KEY_MENU,

    // Mouse keys
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    
    KEY_COUNT,
};

enum Gamepad_Button : s16
{
    GAMEPAD_BUTTON_INVALID = -1,

    GAMEPAD_FACE_UP = 0,
    GAMEPAD_FACE_RIGHT,
    GAMEPAD_FACE_DOWN,
    GAMEPAD_FACE_LEFT,
    GAMEPAD_LB,
    GAMEPAD_RB,
    GAMEPAD_BACK,
    GAMEPAD_START,
    GAMEPAD_GUIDE,
    GAMEPAD_LTHUMB,
    GAMEPAD_RTHUMB,
    GAMEPAD_DPAD_UP,
    GAMEPAD_DPAD_RIGHT,
    GAMEPAD_DPAD_DOWN,
    GAMEPAD_DPAD_LEFT,

    GAMEPAD_BUTTON_COUNT,

    GAMEPAD_TRIANGLE = GAMEPAD_FACE_UP,
    GAMEPAD_CIRCLE = GAMEPAD_FACE_RIGHT,
    GAMEPAD_CROSS = GAMEPAD_FACE_DOWN,
    GAMEPAD_SQUARE = GAMEPAD_FACE_LEFT,

    GAMEPAD_Y = GAMEPAD_FACE_UP,
    GAMEPAD_B = GAMEPAD_FACE_RIGHT,
    GAMEPAD_A = GAMEPAD_FACE_DOWN,
    GAMEPAD_X = GAMEPAD_FACE_LEFT,
};

enum Gamepad_Axis : s16
{
    GAMEPAD_AXIS_INVALID = -1,

    GAMEPAD_LX = 0,
    GAMEPAD_LY,
    GAMEPAD_RX,
    GAMEPAD_RY,
    GAMEPAD_LT,
    GAMEPAD_RT,

    GAMEPAD_AXIS_COUNT,
};

struct Input_Table
{
    bool key_states[KEY_COUNT]; 
    
    s16 virtual_keys[KEY_COUNT]; // key to virtual keycode
    s16 scan_codes[KEY_COUNT];   // key to keyboard scancode
    s16 key_codes[512];          // virtual keycode to key

    s16 mouse_x;
    s16 mouse_y;
    s16 mouse_last_x;
    s16 mouse_last_y;
    s16 mouse_offset_x;
    s16 mouse_offset_y;

    // @Todo: gamepads...
};

inline Input_Table input_table;

void init_input_table();
