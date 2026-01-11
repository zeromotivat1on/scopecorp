#pragma once

enum Window_Event_Type : u8 {
	WINDOW_EVENT_NONE,
    WINDOW_EVENT_FOCUSED,
	WINDOW_EVENT_LOST_FOCUS,
	WINDOW_EVENT_RESIZE,
	WINDOW_EVENT_KEYBOARD,
	WINDOW_EVENT_TEXT_INPUT,
	WINDOW_EVENT_MOUSE_BUTTON,
	WINDOW_EVENT_MOUSE_MOVE,
	WINDOW_EVENT_MOUSE_WHEEL,
	WINDOW_EVENT_FILE_DROP,
	WINDOW_EVENT_QUIT,
};

struct Window_Event {
	Window_Event_Type type;

    union {
        struct { s32 x; s32 y; };
        struct { u32 w; u32 h; };
        struct { Key_Code key_code; bool press; bool repeat; };
        struct { u32 character; };
        struct { s32 scroll_delta; };
        struct { u32 file_drop_count; char *file_drops; };
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
void set_cursor         (Window *w, s32 x, s32 y);
void get_cursor         (Window *w, s32 *x, s32 *y);
void swap_buffers       (Window *w);
void set_vsync          (Window *w, bool enable);
