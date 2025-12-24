#pragma once

enum Window_Event_Type : u8 {
	WINDOW_EVENT_UNKNOWN,
    WINDOW_EVENT_FOCUSED,
	WINDOW_EVENT_LOST_FOCUS,
	WINDOW_EVENT_RESIZE,
	WINDOW_EVENT_KEYBOARD,
	WINDOW_EVENT_TEXT_INPUT,
	WINDOW_EVENT_MOUSE_CLICK,
	WINDOW_EVENT_MOUSE_MOVE,
	WINDOW_EVENT_MOUSE_WHEEL,
	WINDOW_EVENT_FILE_DROP,
	WINDOW_EVENT_QUIT,
};

enum Window_Event_Input_Bits : u8 {
    WINDOW_EVENT_PRESS_BIT   = 0x1,
    WINDOW_EVENT_RELEASE_BIT = 0x2,
    WINDOW_EVENT_REPEAT_BIT  = 0x4,
};

struct Window_Event {
	Window_Event_Type type = WINDOW_EVENT_UNKNOWN;

    union {
        struct { s16 x; s16 y; };
        struct { u16 w; u16 h; };
        struct { u8 input_bits; Key_Code key_code; };
        struct { u32 character; };
        struct { s16 scroll_delta; };
        struct { u16 file_drop_count; char *file_drops; };
    };
};

struct Window {
    void *user_data = null;

	u16 width  = 0;
	u16 height = 0;

    bool vsync = false;
    bool focused = false;
	bool cursor_locked = false;
	bool last_cursor_locked = false;

    Array <Window_Event> events; // saved events after poll_events call

    void *handle = null;
    
#ifdef WIN32
	struct Win32_Window *win32 = null;
#endif
};

Window *new_window (u16 width, u16 height, const char *name, s16 x = 0, s16 y = 0);

void destroy            (Window *w);
void poll_events        (Window *w);
void close              (Window *w);
bool is_valid           (Window *w);
bool set_title          (Window *w, const char *title);
void lock_cursor        (Window *w, bool lock);
void set_cursor         (Window *w, s16 x, s16 y);
void swap_buffers       (Window *w);
void set_vsync          (Window *w, bool enable);
