#pragma once

inline constexpr u32 MAX_WINDOW_DROP_COUNT = 64;

typedef void(*Window_Event_Callback)(struct Window *window, struct Window_Event *event);

enum Window_Event_Type : u8 {
	WINDOW_EVENT_UNKNOWN,
	WINDOW_EVENT_RESIZE,
	WINDOW_EVENT_KEYBOARD,
	WINDOW_EVENT_TEXT_INPUT,
	WINDOW_EVENT_MOUSE,
	WINDOW_EVENT_FILE_DROP,
	WINDOW_EVENT_QUIT,
};

struct Window_Event {
	Window_Event_Type type;
	bool key_press;
	bool key_release;
	bool key_repeat;
    bool with_ctrl;
    bool with_shift;
    bool with_alt;
	s16 key_code;
    char *file_drops;
    s16 file_drop_count;
    s16 scroll_delta;
	u32 character;
    s16 prev_width;
    s16 prev_height;
};

struct Window {
    void *user_data;
	Window_Event_Callback event_callback;

	s16 width;
	s16 height;

    bool focused;
	bool cursor_locked;
	bool last_cursor_locked;

#if WIN32
	struct Win32_Window *win32;
#endif
};

inline Window *window = null;

inline constexpr s32 MAX_WINDOW_EVENT_QUEUE_SIZE = 32; // max window events per frame
inline Window_Event window_event_queue[MAX_WINDOW_EVENT_QUEUE_SIZE];
inline s32 window_event_queue_size = 0;

Window *os_create_window(s32 w, s32 h, const char *name, s32 x, s32 y, void *user_data = null);
void os_register_window_callback(Window *window, Window_Event_Callback callback);
void os_destroy_window(Window *window);
void os_poll_window_events(Window *window);
void os_close_window(Window *window);
bool os_window_alive(Window *window);
bool os_set_window_title(Window *window, const char *title);
void os_lock_window_cursor(Window *window, bool lock);
void os_swap_window_buffers(Window *window);

void os_set_vsync(bool enable);
