#pragma once

inline constexpr u32 MAX_WINDOW_DROP_COUNT = 64;

struct Window;
struct Window_Event;

typedef void(*Window_Event_Callback)(const Window &window, const Window_Event &event);

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
    u16 file_drop_count;
    s16 scroll_delta;
	u32 character;
    u16 prev_width;
    u16 prev_height;
};

struct Window {
    void *user_data = null;
	Window_Event_Callback event_callback = null;

	u16 width  = 0;
	u16 height = 0;

    bool vsync = false;
    bool focused = false;
	bool cursor_locked = false;
	bool last_cursor_locked = false;

#if WIN32
	struct Win32_Window *win32 = null;
#endif
};

inline Window window;

inline constexpr s32 MAX_WINDOW_EVENT_QUEUE_SIZE = 32; // max window events per frame
inline Window_Event window_event_queue[MAX_WINDOW_EVENT_QUEUE_SIZE];
inline s32 window_event_queue_size = 0;

bool os_create_window(u16 width, u16 height, const char *name, s16 x, s16 y, Window &w);
void os_set_window_user_data(Window &w, void *user_data);
void os_register_window_callback(Window &w, Window_Event_Callback callback);
void os_destroy_window(Window &w);
void os_poll_window_events(Window &w);
void os_close_window(Window &w);
bool os_window_alive(Window &w);
bool os_set_window_title(Window &w, const char *title);
void os_lock_window_cursor(Window &w, bool lock);
void os_swap_window_buffers(Window &w);

void os_set_window_vsync(Window &w, bool enable);
