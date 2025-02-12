#include "pch.h"
#include "window.h"
#include "win32_window.h"
#include "input.h"
#include <windowsx.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static const char* window_prop_name = "win32_window";

static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    auto* window = (Window*)GetProp(hwnd, window_prop_name);
    if (!window) return DefWindowProc(hwnd, umsg, wparam, lparam);

    Window_Event event = {0};
    
    switch(umsg)
    {
    case WM_SIZE:
    {
        window->width = LOWORD(lparam);
        window->height = HIWORD(lparam);

        event.type = EVENT_RESIZE;
        
        // @Robustness: ensure we have only one resize event in queue.
        // @Cleanup: looks like a hack, need to figure this out.
        s32 resized_event_idx = INVALID_INDEX;
        for (s32 i = 0; i < window_event_queue_size; ++i)
            if (window_event_queue[i].type == EVENT_RESIZE)
                resized_event_idx = i;

        if (resized_event_idx == INVALID_INDEX)
        {
            assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
            window_event_queue[window_event_queue_size++] = event;
        }
        else
        {
            window_event_queue[resized_event_idx] = event;
        }
        
        break;
    }
    case WM_KEYDOWN:
    {
        const s32 repeat = lparam & 0x40000000;
        if (repeat == 0)
        {
            event.type = EVENT_KEYBOARD;
            event.key_pressed = true;
            event.key_code = input_table.key_codes[wparam];

            assert(event.key_code > 0);
            input_table.key_states[event.key_code] = event.key_pressed;

            assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
            window_event_queue[window_event_queue_size++] = event;
        }
        
        break;
    }
    case WM_KEYUP:
    {
        event.type = EVENT_KEYBOARD;
        event.key_pressed = false;
        event.key_code = input_table.key_codes[wparam];

        assert(event.key_code > 0);
        input_table.key_states[event.key_code] = event.key_pressed;

        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;
        
        break;
    }
    case WM_CHAR:
    {
        event.type = EVENT_TEXT_INPUT;
        event.character = (u32)wparam;

        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;

        break;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        event.type = EVENT_MOUSE;
        event.key_pressed = umsg == WM_LBUTTONDOWN;
        event.key_code = MOUSE_LEFT;

        input_table.key_states[event.key_code] = event.key_pressed;

        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;

        break;
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
        event.type = EVENT_MOUSE;
        event.key_pressed = umsg == WM_RBUTTONDOWN;
        event.key_code = MOUSE_RIGHT;

        input_table.key_states[event.key_code] = event.key_pressed;
        
        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;

        break;
    }
    case WM_MBUTTONUP:
    case WM_MBUTTONDOWN:
    {
        event.type = EVENT_MOUSE;
        event.key_pressed = umsg == WM_MBUTTONDOWN;
        event.key_code = MOUSE_MIDDLE;

        input_table.key_states[event.key_code] = event.key_pressed;
        
        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;

        break;
    }
    case WM_MOUSEMOVE:
    {
        input_table.mouse_x = GET_X_LPARAM(lparam);
        input_table.mouse_y = GET_Y_LPARAM(lparam);        
        break;
    }
        //case WM_MOUSEWHEEL:
        //    win->mouse_axes[MOUSE_SCROLL_X] = 0;
        //    win->mouse_axes[MOUSE_SCROLL_Y] = GET_WHEEL_DELTA_WPARAM(wparam);
        //    break;

    case WM_QUIT:
    case WM_CLOSE:
    {
        event.type = EVENT_QUIT;
        assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }
    default:
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }
    
    return 0;
}

Window* create_window(s32 w, s32 h, const char* name, s32 x, s32 y)
{
    Window* window = new Window();
    window->win32 = new Win32_Window();
    
    window->win32->class_name = "win32_window";

    WNDCLASSEX wclass = {0};
    wclass.cbSize = sizeof(wclass);
    wclass.style = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc = win32_window_proc;
    wclass.hInstance = window->win32->hinstance;
    wclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wclass.lpszClassName = window->win32->class_name;
    
    window->win32->class_atom = RegisterClassEx(&wclass);
    window->win32->hinstance = (HINSTANCE)&__ImageBase;

    window->win32->hwnd = CreateWindowEx(
        0,
        window->win32->class_name,
        name,
        WS_OVERLAPPEDWINDOW,
        x,
        y,
        w,
        h,
        NULL,
        NULL,
        window->win32->hinstance,
        NULL
    );

    if (window->win32->hwnd == NULL) return null;

    window->win32->hdc = GetDC(window->win32->hwnd);
    if (window->win32->hdc == NULL) return null;

    SetProp(window->win32->hwnd, window_prop_name, window);
    
    return window;
}

void register_event_callback(Window* window, Window_Event_Callback callback)
{
    window->event_callback = callback;
}

void destroy(Window* window)
{
    ReleaseDC(window->win32->hwnd, window->win32->hdc);
    DestroyWindow(window->win32->hwnd);
    UnregisterClass(window->win32->class_name, window->win32->hinstance);
    delete window->win32;
    delete window;
}

void poll_events(Window* window)
{
    input_table.mouse_offset_x = 0;
    input_table.mouse_offset_y = 0;
    
    MSG msg = {0};
    msg.hwnd = window->win32->hwnd;

    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    input_table.mouse_offset_x = input_table.mouse_last_x - input_table.mouse_x;
    input_table.mouse_offset_y = input_table.mouse_last_y - input_table.mouse_y;
    
    if (window->cursor_locked)
    {
        POINT point;
        point.x = input_table.mouse_last_x = window->width / 2;
        point.y = input_table.mouse_last_y = window->height / 2;
        
        ClientToScreen(window->win32->hwnd, &point);
        SetCursorPos(point.x, point.y);
    }
    else
    {
        input_table.mouse_last_x = input_table.mouse_x;
        input_table.mouse_last_y = input_table.mouse_y;
    }
    
    for (s32 i = 0; i < window_event_queue_size; ++i)
        window->event_callback(window, window_event_queue + i);
    window_event_queue_size = 0;
}

void close(Window* window)
{
    PostMessage(window->win32->hwnd, WM_CLOSE, 0, 0);
}

bool alive(Window* window)
{
    return IsWindow(window->win32->hwnd);
}

void lock_cursor(Window* window, bool lock)
{
    window->cursor_locked = lock;
    
    if (lock)
    {
        RECT rect;
        if (GetWindowRect(window->win32->hwnd, &rect)) ClipCursor(&rect);
        ShowCursor(false);
    }
    else
    {
        ClipCursor(NULL);
        ShowCursor(true);
    }
}
