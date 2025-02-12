#pragma once

typedef void(*Window_Event_Callback)(struct Window* window, struct Window_Event* event);

struct Window
{
    Window_Event_Callback event_callback;
    
    s16 width;
    s16 height;

    bool cursor_locked = false;
    
#if WIN32
    struct Win32_Window* win32;
#endif
};

enum Event_Type : u8
{
    EVENT_UNKNOWN,
    EVENT_RESIZE,
    EVENT_KEYBOARD,
    EVENT_TEXT_INPUT,
    EVENT_MOUSE,
    EVENT_QUIT,
};

struct Window_Event
{
    Event_Type type;
    bool key_pressed;
    s16 key_code;
    u32 character;
};

inline constexpr s32 MAX_WINDOW_EVENT_QUEUE_SIZE = 32; // max window events per frame
inline Window_Event window_event_queue[MAX_WINDOW_EVENT_QUEUE_SIZE];
inline s32 window_event_queue_size = 0;

Window* create_window(s32 w, s32 h, const char* name, s32 x, s32 y);
void register_event_callback(Window* window, Window_Event_Callback callback);
void destroy(Window* window);
void poll_events(Window* window);
void close(Window* window);
bool alive(Window* window);
void lock_cursor(Window* window, bool lock);
