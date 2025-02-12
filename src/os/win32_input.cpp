#include "pch.h"
#include "input.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void init_input_table()
{
    input_table = {0};

    input_table.virtual_keys[KEY_SPACE]         = VK_SPACE;
    input_table.virtual_keys[KEY_APOSTROPHE]    = VK_OEM_7;
    input_table.virtual_keys[KEY_COMMA]         = VK_OEM_COMMA;
    input_table.virtual_keys[KEY_MINUS]         = VK_OEM_MINUS;
    input_table.virtual_keys[KEY_PERIOD]        = VK_OEM_PERIOD;
    input_table.virtual_keys[KEY_SLASH]         = VK_OEM_2;
    input_table.virtual_keys[KEY_0]             = 0x30;
    input_table.virtual_keys[KEY_1]             = 0x31;
    input_table.virtual_keys[KEY_2]             = 0x32;
    input_table.virtual_keys[KEY_3]             = 0x33;
    input_table.virtual_keys[KEY_4]             = 0x34;
    input_table.virtual_keys[KEY_5]             = 0x35;
    input_table.virtual_keys[KEY_6]             = 0x36;
    input_table.virtual_keys[KEY_7]             = 0x37;
    input_table.virtual_keys[KEY_8]             = 0x38;
    input_table.virtual_keys[KEY_9]             = 0x39;
    input_table.virtual_keys[KEY_SEMICOLON]     = VK_OEM_1;
    input_table.virtual_keys[KEY_EQUAL]         = VK_OEM_PLUS;
    input_table.virtual_keys[KEY_A]             = 0x41;
    input_table.virtual_keys[KEY_B]             = 0x42;
    input_table.virtual_keys[KEY_C]             = 0x43;
    input_table.virtual_keys[KEY_D]             = 0x44;
    input_table.virtual_keys[KEY_E]             = 0x45;
    input_table.virtual_keys[KEY_F]             = 0x46;
    input_table.virtual_keys[KEY_G]             = 0x47;
    input_table.virtual_keys[KEY_H]             = 0x48;
    input_table.virtual_keys[KEY_I]             = 0x49;
    input_table.virtual_keys[KEY_J]             = 0x4A;
    input_table.virtual_keys[KEY_K]             = 0x4B;
    input_table.virtual_keys[KEY_L]             = 0x4C;
    input_table.virtual_keys[KEY_M]             = 0x4D;
    input_table.virtual_keys[KEY_N]             = 0x4E;
    input_table.virtual_keys[KEY_O]             = 0x4F;
    input_table.virtual_keys[KEY_P]             = 0x50;
    input_table.virtual_keys[KEY_Q]             = 0x51;
    input_table.virtual_keys[KEY_R]             = 0x52;
    input_table.virtual_keys[KEY_S]             = 0x53;
    input_table.virtual_keys[KEY_T]             = 0x54;
    input_table.virtual_keys[KEY_U]             = 0x55;
    input_table.virtual_keys[KEY_V]             = 0x56;
    input_table.virtual_keys[KEY_W]             = 0x57;
    input_table.virtual_keys[KEY_X]             = 0x58;
    input_table.virtual_keys[KEY_Y]             = 0x59;
    input_table.virtual_keys[KEY_Z]             = 0x5A;
    input_table.virtual_keys[KEY_LEFT_BRACKET]  = VK_OEM_4;
    input_table.virtual_keys[KEY_BACKSLASH]     = VK_OEM_5;
    input_table.virtual_keys[KEY_RIGHT_BRACKET] = VK_OEM_6;
    input_table.virtual_keys[KEY_GRAVE_ACCENT]  = VK_OEM_3;
    input_table.virtual_keys[KEY_ESCAPE]        = VK_ESCAPE;
    input_table.virtual_keys[KEY_ENTER]         = VK_RETURN;
    input_table.virtual_keys[KEY_TAB]           = VK_TAB;
    input_table.virtual_keys[KEY_BACKSPACE]     = VK_BACK;
    input_table.virtual_keys[KEY_INSERT]        = VK_INSERT;
    input_table.virtual_keys[KEY_DELETE]        = VK_DELETE;
    input_table.virtual_keys[KEY_RIGHT]         = VK_RIGHT;
    input_table.virtual_keys[KEY_LEFT]          = VK_LEFT;
    input_table.virtual_keys[KEY_DOWN]          = VK_DOWN;
    input_table.virtual_keys[KEY_UP]            = VK_UP;
    input_table.virtual_keys[KEY_PAGE_UP]       = VK_PRIOR;
    input_table.virtual_keys[KEY_PAGE_DOWN]     = VK_NEXT;
    input_table.virtual_keys[KEY_HOME]          = VK_HOME;
    input_table.virtual_keys[KEY_END]           = VK_END;
    input_table.virtual_keys[KEY_CAPS_LOCK]     = VK_CAPITAL;
    input_table.virtual_keys[KEY_SCROLL_LOCK]   = VK_SCROLL;
    input_table.virtual_keys[KEY_NUM_LOCK]      = VK_NUMLOCK;
    input_table.virtual_keys[KEY_PRINT_SCREEN]  = VK_SNAPSHOT;
    input_table.virtual_keys[KEY_PAUSE]         = VK_PAUSE;
    input_table.virtual_keys[KEY_F1]            = VK_F1;
    input_table.virtual_keys[KEY_F2]            = VK_F2;
    input_table.virtual_keys[KEY_F3]            = VK_F3;
    input_table.virtual_keys[KEY_F4]            = VK_F4;
    input_table.virtual_keys[KEY_F5]            = VK_F5;
    input_table.virtual_keys[KEY_F6]            = VK_F6;
    input_table.virtual_keys[KEY_F7]            = VK_F7;
    input_table.virtual_keys[KEY_F8]            = VK_F8;
    input_table.virtual_keys[KEY_F9]            = VK_F9;
    input_table.virtual_keys[KEY_F10]           = VK_F10;
    input_table.virtual_keys[KEY_F11]           = VK_F11;
    input_table.virtual_keys[KEY_F12]           = VK_F12;
    input_table.virtual_keys[KEY_F13]           = VK_F13;
    input_table.virtual_keys[KEY_F14]           = VK_F14;
    input_table.virtual_keys[KEY_F15]           = VK_F15;
    input_table.virtual_keys[KEY_F16]           = VK_F16;
    input_table.virtual_keys[KEY_F17]           = VK_F17;
    input_table.virtual_keys[KEY_F18]           = VK_F18;
    input_table.virtual_keys[KEY_F19]           = VK_F19;
    input_table.virtual_keys[KEY_F20]           = VK_F20;
    input_table.virtual_keys[KEY_F21]           = VK_F21;
    input_table.virtual_keys[KEY_F22]           = VK_F22;
    input_table.virtual_keys[KEY_F23]           = VK_F23;
    input_table.virtual_keys[KEY_F24]           = VK_F24;
    input_table.virtual_keys[KEY_F25]           = KEY_UNKNOWN;
    input_table.virtual_keys[KEY_KP_0]          = VK_NUMPAD0;
    input_table.virtual_keys[KEY_KP_1]          = VK_NUMPAD1;
    input_table.virtual_keys[KEY_KP_2]          = VK_NUMPAD2;
    input_table.virtual_keys[KEY_KP_3]          = VK_NUMPAD3;
    input_table.virtual_keys[KEY_KP_4]          = VK_NUMPAD4;
    input_table.virtual_keys[KEY_KP_5]          = VK_NUMPAD5;
    input_table.virtual_keys[KEY_KP_6]          = VK_NUMPAD6;
    input_table.virtual_keys[KEY_KP_7]          = VK_NUMPAD7;
    input_table.virtual_keys[KEY_KP_8]          = VK_NUMPAD8;
    input_table.virtual_keys[KEY_KP_9]          = VK_NUMPAD9;
    input_table.virtual_keys[KEY_KP_DECIMAL]    = VK_DECIMAL;
    input_table.virtual_keys[KEY_KP_DIVIDE]     = VK_DIVIDE;
    input_table.virtual_keys[KEY_KP_MULTIPLY]   = VK_MULTIPLY;
    input_table.virtual_keys[KEY_KP_SUBTRACT]   = VK_SUBTRACT;
    input_table.virtual_keys[KEY_KP_ADD]        = VK_ADD;
    input_table.virtual_keys[KEY_KP_ENTER]      = KEY_UNKNOWN;
    input_table.virtual_keys[KEY_KP_EQUAL]      = KEY_UNKNOWN;
    input_table.virtual_keys[KEY_SHIFT]         = VK_SHIFT;
    input_table.virtual_keys[KEY_CTRL]          = VK_CONTROL;
    input_table.virtual_keys[KEY_ALT]           = VK_MENU;
    input_table.virtual_keys[KEY_LEFT_SHIFT]    = VK_LSHIFT;
    input_table.virtual_keys[KEY_LEFT_CTRL]     = VK_LCONTROL;
    input_table.virtual_keys[KEY_LEFT_ALT]      = VK_LMENU;
    input_table.virtual_keys[KEY_LEFT_SUPER]    = KEY_UNKNOWN;
    input_table.virtual_keys[KEY_RIGHT_SHIFT]   = VK_RSHIFT;
    input_table.virtual_keys[KEY_RIGHT_CTRL]    = VK_RCONTROL;
    input_table.virtual_keys[KEY_RIGHT_ALT]     = VK_RMENU;
    input_table.virtual_keys[KEY_RIGHT_SUPER]   = KEY_UNKNOWN;
    input_table.virtual_keys[KEY_MENU]          = KEY_UNKNOWN;

    for (s16 key = 0; key < KEY_COUNT; ++key)
    {
        const s32 virtual_key = input_table.virtual_keys[key];
        if (virtual_key == KEY_UNKNOWN) continue;

        input_table.key_codes[virtual_key] = key;
        
        const UINT scan_code = MapVirtualKey(virtual_key, MAPVK_VK_TO_VSC);
        if (scan_code == 0) continue;
        
        input_table.scan_codes[key] = scan_code;
    }    
}
