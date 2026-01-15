#include "pch.h"
#include "profile.h"
#include "win32.h"
#include "file_system.h"
#include "atomic.h"
#include "input.h"
#include "memory.h"
#include "sync.h"
#include "thread.h"
#include "time.h"
#include "window.h"
#include "editor.h"
#include <windowsx.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <intrin.h>

static const auto LOG_IDENT_WIN32 = S("win32");

static constexpr u32 INVALID_THREAD_RESULT = ((DWORD)-1);

const u32 WAIT_INFINITE = INFINITE;

const Thread    THREAD_NONE    = NULL;
const Mutex     MUTEX_NONE     = NULL;
const Semaphore SEMAPHORE_NONE = NULL;
const File      FILE_NONE      = INVALID_HANDLE_VALUE;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static const char *window_prop_name = "win32_window";

static SYSTEM_INFO System_info = {};

u64 get_page_size() {
    if (System_info.dwPageSize == 0) {
        GetSystemInfo(&System_info);
    }

    return System_info.dwPageSize;
}

u64 get_allocation_granularity() {
    if (System_info.dwAllocationGranularity == 0) {
        GetSystemInfo(&System_info);
    }

    return System_info.dwAllocationGranularity;
}

String get_process_directory() {
    char path[MAX_PATH_LENGTH];
    GetModuleFileNameA(null, path, carray_count(path));
    PathRemoveFileSpecA(path);

    auto s = copy_string(path, __temporary_allocator);
    s = fix_directory_delimiters(s);
    
    s.data[s.size] = '/';
    s.size += 1;
    
    return s;
}

void set_process_cwd(String path) {
    auto cstr = temp_c_string(path);
    SetCurrentDirectoryA(cstr);    
}

static Source_Code_Location get_stack_frame_location(HANDLE process, void *address) {
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];

    SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    union { DWORD64 a; DWORD b; } displacement;
    IMAGEHLP_LINE line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    DWORD line_displacement;
    const bool got_line = SymGetLineFromAddr(process, (DWORD64)address, &line_displacement, &line);

    if (!SymFromAddr(process, (DWORD64)address, &displacement.a, symbol)) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to retreive symbol from address 0x%X", GetLastError(), address);
        return {};
    }

    auto func_name = cstring_copy(symbol->Name, __temporary_allocator);

    if (got_line) {
        auto file_name = cstring_copy(line.FileName, __temporary_allocator);
        return { .file = file_name, .function = func_name, .line = (s32)line.LineNumber };
    }

    return { .file = null, .function = func_name, .line = 0 };
}

Array <Source_Code_Location> get_current_callstack() {
    auto current_process = GetCurrentProcess();
    HANDLE process;
    
    if (!DuplicateHandle(current_process, current_process, current_process, &process, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to duplicate process handle", GetLastError());
        return {};
    }

    if (!SymInitialize(process, NULL, TRUE)) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to initialized sym", GetLastError());
        return {};
    }
    
    SymSetOptions(SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_LOAD_LINES);
    
    void *frames[256];
    const s32 frame_count = CaptureStackBackTrace(0, carray_count(frames), frames, NULL);

    Array <Source_Code_Location> callstack;
    callstack.allocator = __temporary_allocator;
    array_realloc(callstack, frame_count);

    for (s32 i = 0; i < frame_count; ++i) {
        array_add(callstack, get_stack_frame_location(process, frames[i]));
    }
    
    return callstack;
}

static u32 win32_access_bits(u32 bits) {
    u32 out = 0;

    if (bits & FILE_READ_BIT)  out |= GENERIC_READ;
    if (bits & FILE_WRITE_BIT) out |= GENERIC_WRITE;

    return out;
}

static u32 win32_share_bits(u32 bits) {
    u32 out = 0;

    if (bits & FILE_READ_BIT)  out |= FILE_SHARE_READ;
    if (bits & FILE_WRITE_BIT) out |= FILE_SHARE_WRITE;

    return out;
}

static u32 win32_open_type(u32 bits) {
    if (bits & FILE_NEW_BIT) return CREATE_NEW;    
    return OPEN_EXISTING;
}

File open_file(String path, u32 bits, bool log_error) {
    const char *cpath = temp_c_string(path);
	HANDLE handle = CreateFile(cpath, win32_access_bits(bits), win32_share_bits(bits),
                               NULL, win32_open_type(bits), FILE_ATTRIBUTE_NORMAL, NULL);

    if (log_error && handle == INVALID_HANDLE_VALUE) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to open file %s", GetLastError(), cpath);
    }
    
    return handle;
}

bool close_file(File handle) { return CloseHandle(handle); }

u64 get_file_size(File handle) {
	LARGE_INTEGER size;
	if (!GetFileSizeEx(handle, &size)) return -1;
	return size.QuadPart;
}

u64 read_file(File handle, u64 size, void *buffer) {
    if (size == 0) return 0;
    
    DWORD read;
	if (ReadFile(handle, buffer, (DWORD)size, &read, NULL)) return read;
    return 0;
}

u64 write_file(File handle, u64 size, const void *buffer) {
    if (size == 0) return 0;
 
    DWORD written;
	if (WriteFile(handle, buffer, (DWORD)size, &written, NULL)) return written;
    return 0;
}

bool set_file_ptr(File handle, s64 position) {
    LARGE_INTEGER move_distance;
    move_distance.QuadPart = position;
    return SetFilePointerEx(handle, move_distance, NULL, FILE_BEGIN);
}

bool path_file_exists(String path) {
    const auto cpath = temp_c_string(path);
    return PathFileExistsA(cpath);
}

void visit_directory(String path, void (*callback) (const File_Callback_Data *),
                     bool recursive, void *user_data) {
    WIN32_FIND_DATA find_data;
    TCHAR file_path     [MAX_PATH + 0];
    TCHAR directory_mask[MAX_PATH + 3];

    auto directory = temp_c_string(path);
    PathCombineA(directory_mask, directory, "*");
    
    HANDLE file = FindFirstFileA(directory_mask, &find_data);
    if (file == INVALID_HANDLE_VALUE) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to search directory %s", GetLastError(), directory_mask);
        return;
    }

    File_Callback_Data callback_data;
    callback_data.user_data = user_data;
    
    do {
        const char *file_name = find_data.cFileName;
        PathCombineA(file_path, directory, file_name);           

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (file_name[0] == '.' || (file_name[0] == '.' && file_name[1] == '.')) continue;
            if (recursive) {
                visit_directory(make_string(file_path), callback, recursive, user_data);
            }
        } else {
            callback_data.path = fix_directory_delimiters(make_string(file_path));
            callback_data.size = (find_data.nFileSizeHigh * (MAXDWORD + 1)) + find_data.nFileSizeLow;

            ULARGE_INTEGER last_write_time;
            last_write_time.LowPart  = find_data.ftLastWriteTime.dwLowDateTime;
            last_write_time.HighPart = find_data.ftLastWriteTime.dwHighDateTime;
            callback_data.last_write_time = last_write_time.QuadPart;
            
            callback(&callback_data);
        }
    } while (FindNextFile(file, &find_data));

    FindClose(file);
}

String extract_file_from_path(String path, Allocator alc) {
    auto cpath = to_c_string(path, alc);
    PathStripPath(cpath);
    return make_string(cpath);
}

s64 get_file_ptr(File handle) {
    LARGE_INTEGER position      = {0};
    LARGE_INTEGER move_distance = {0};
    if (SetFilePointerEx(handle, move_distance, &position, FILE_CURRENT))
        return position.QuadPart;
    return INDEX_NONE;
}

void *heap_alloc (u64 size) { return HeapAlloc(GetProcessHeap(), 0, size); }
bool  heap_free  (void *p)  { return HeapFree (GetProcessHeap(), 0, p); }

void *virtual_reserve  (void *addr, u64 size) { return VirtualAlloc(addr, size, MEM_RESERVE, PAGE_READWRITE); }
void *virtual_commit   (void *vm, u64 size)   { return VirtualAlloc(vm, size, MEM_COMMIT, PAGE_READWRITE); }
bool  virtual_decommit (void *vm, u64 size)   { return VirtualFree(vm, size, MEM_DECOMMIT); }
bool  virtual_release  (void *vm)             { return VirtualFree(vm, 0, MEM_RELEASE); }

static BOOL win32_wait_res_check(void *handle, DWORD res) {
	switch (res) {
	case WAIT_OBJECT_0:
		return true;
	case WAIT_ABANDONED:
		log(LOG_VERBOSE, "Object %p was not released before owning thread termination", handle);
		return false;
	case WAIT_TIMEOUT:
		log(LOG_VERBOSE, "Wait time for object %p elapsed", handle);
		return false;
	case WAIT_FAILED:
		log(LOG_ERROR, "Failed to wait for object %p with error code %u", handle, GetLastError());
		return false;
	default:
		log(LOG_ERROR, "Unknown wait result %u for object %p", res, handle);
		return false;
	}
}

u64  get_current_thread_id ()       { return GetCurrentThreadId(); }
void sleep                 (u32 ms) { Sleep(ms); }

bool is_active_thread(Thread handle) {
	DWORD code;
	if (!GetExitCodeThread(handle, &code)) return false;
	return code == STILL_ACTIVE;
}

static u32 win32_thread_create_type(u32 bits) {
    if (bits & THREAD_SUSPENDED_BIT) return CREATE_SUSPENDED;
    return 0;
}

Thread create_thread(u32 (*entry)(void *), u32 bits, void *user_data) {
	return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)entry, user_data,
                        win32_thread_create_type(bits), NULL);
}

bool resume_thread  (Thread handle) { return ResumeThread(handle); }
bool suspend_thread (Thread handle) { return SuspendThread(handle); }

bool terminate_thread(Thread handle) {
	DWORD code;
	if (!GetExitCodeThread(handle, &code)) return false;
	return TerminateThread(handle, code);
}

Semaphore create_semaphore  (s32 init_count, s32 max_count)                { return CreateSemaphore(NULL, (LONG)init_count, (LONG)max_count, NULL); }
bool      release_semaphore (Semaphore handle, s32 count, s32 *prev_count) { return ReleaseSemaphore(handle, count, (LPLONG)prev_count); }
bool      wait_semaphore    (Semaphore handle, u32 ms)                     { return win32_wait_res_check(handle, WaitForSingleObjectEx(handle, ms, FALSE)); }

Mutex create_mutex  (bool signaled)        { return CreateMutex(NULL, (LONG)signaled, NULL); }
bool  release_mutex (Mutex handle)         { return ReleaseMutex(handle); }
bool  wait_mutex    (Mutex handle, u32 ms) { return win32_wait_res_check(handle, WaitForSingleObjectEx(handle, ms, FALSE)); }

Critical_Section create_cs(u32 spin_count) {
    static u32 count = 0;
    static CRITICAL_SECTION sections[64];

    Assert(count < carray_count(sections));

    auto pcs = sections + count;
    count += 1;
        
	if (spin_count > 0) {
		InitializeCriticalSectionAndSpinCount(pcs, spin_count);
    } else {
		InitializeCriticalSection(pcs);
    }

    return pcs;
}

void enter_cs     (Critical_Section handle) { EnterCriticalSection((LPCRITICAL_SECTION)handle); }
bool try_enter_cs (Critical_Section handle) { return TryEnterCriticalSection((LPCRITICAL_SECTION)handle); }
void leave_cs     (Critical_Section handle) { LeaveCriticalSection((LPCRITICAL_SECTION)handle); }
void delete_cs    (Critical_Section handle) { DeleteCriticalSection((LPCRITICAL_SECTION)handle); }

s32   atomic_swap      (s32 *dst, s32 val)                { return InterlockedExchange((LONG *)dst, val); }
void *atomic_swap      (void **dst, void *val)            { return InterlockedExchangePointer(dst, val); }
s32   atomic_cmp_swap  (s32 *dst, s32 val, s32 cmp)       { return InterlockedCompareExchange((LONG *)dst, val, cmp); }
void *atomic_cmp_swap  (void **dst, void *val, void *cmp) { return InterlockedCompareExchangePointer(dst, val, cmp); }
s32   atomic_add       (s32 *dst, s32 val)                { return InterlockedAdd((LONG *)dst, val); }
s32   atomic_increment (s32 *dst)                         { return InterlockedIncrement((LONG *)dst); }
s32   atomic_decrement (s32 *dst)                         { return InterlockedDecrement((LONG *)dst); }

u64 get_time_since_boot_ms() { return GetTickCount64(); }

u64 get_perf_counter() {
	LARGE_INTEGER counter;
	while (!QueryPerformanceCounter(&counter)) {}
	return counter.QuadPart;
}

u64 get_perf_hz() {
	static u64 hz = 0;

	if (hz == 0) {
		LARGE_INTEGER frequency;
		while (!QueryPerformanceFrequency(&frequency)) {}
		hz = frequency.QuadPart;
	}

	return hz;
}

u64 get_perf_hz_ms() { static u64 hz = get_perf_hz()    / 1000; return hz; }
u64 get_perf_hz_us() { static u64 hz = get_perf_hz_ms() / 1000; return hz; }

static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {        
	auto *w = (Window *)GetProp(hwnd, window_prop_name);
	if (!w) return DefWindowProc(hwnd, umsg, wparam, lparam);
    
	Window_Event event = {};
    
	switch (umsg) {
    case WM_SETFOCUS: {
        event.type = WINDOW_EVENT_FOCUSED;

        w->focused = true;
        w->cursor_locked = w->last_cursor_locked;

        array_add(w->events, event);
        break;
    }
    case WM_KILLFOCUS: {
        event.type = WINDOW_EVENT_LOST_FOCUS;
        
        w->focused = false;
        w->last_cursor_locked = w->cursor_locked;
        w->cursor_locked = false;

        array_add(w->events, event);
        break;
    }
	case WM_SIZE: {
        event.type = WINDOW_EVENT_RESIZE;
        event.w = LOWORD(lparam);
        event.h = HIWORD(lparam);
        
		w->width  = LOWORD(lparam);
		w->height = HIWORD(lparam);

        // Replace resize event if its present in array to skip unnecessary resizes.
		s32 resize_event_index = INDEX_NONE;
        s32 index = 0;
		For (w->events) {
			if (it.type == WINDOW_EVENT_RESIZE) {
                resize_event_index = index;
                break;
            }
            
            index += 1;
        }
        
		if (resize_event_index == INDEX_NONE) {
            array_add(w->events, event);
		} else {
			w->events[resize_event_index] = event;
		}

		break;
	}
    case WM_SYSKEYDOWN:
	case WM_KEYDOWN: {
        event.type = WINDOW_EVENT_KEYBOARD;
        event.repeat = lparam & 0x40000000;
        event.press  = !event.repeat;
        event.key_code = vkey_to_key_code((u32)wparam);
        Assert(event.key_code > 0);
        array_add(w->events, event);
		break;
	}
    case WM_SYSKEYUP:
	case WM_KEYUP: {
		event.type = WINDOW_EVENT_KEYBOARD;
        event.press = false;
		event.key_code = vkey_to_key_code((u32)wparam);
		Assert(event.key_code > 0);
        array_add(w->events, event);
		break;
	}
	case WM_CHAR: {
		event.type = WINDOW_EVENT_TEXT_INPUT;
		event.character = (u32)wparam;
        array_add(w->events, event);
		break;
	}
    case WM_INPUT: {
        static BYTE lpb[sizeof(RAWINPUT)];

        UINT dwSize = sizeof(RAWINPUT);
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

        auto raw = (RAWINPUT*)lpb;
        
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            auto mouse = raw->data.mouse;

            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                log(LOG_IDENT_WIN32, LOG_ERROR, "Mouse absolute move, unhandled");
            } else if (mouse.lLastX != 0 || mouse.lLastY != 0) {
                event.type = WINDOW_EVENT_MOUSE_MOVE;
                event.x = mouse.lLastX;
                event.y = mouse.lLastY;
                array_add(w->events, event);
            }

            // @Cleanup: maaaybe we could use some sort of lookup table to skip some ifs...
            if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = true;
                event.key_code = MOUSE_LEFT;
                array_add(w->events, event);
            }

            if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = false;
                event.key_code = MOUSE_LEFT;
                array_add(w->events, event);
            }

            if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = true;
                event.key_code = MOUSE_RIGHT;
                array_add(w->events, event);
            }

            if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = false;
                event.key_code = MOUSE_RIGHT;
                array_add(w->events, event);
            }

            if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = true;
                event.key_code = MOUSE_MIDDLE;
                array_add(w->events, event);
            }

            if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
                event.type = WINDOW_EVENT_MOUSE_BUTTON;
                event.press = false;
                event.key_code = MOUSE_RIGHT;
                array_add(w->events, event);
            }

            if ((mouse.usButtonFlags & RI_MOUSE_WHEEL) || (mouse.usButtonFlags & RI_MOUSE_HWHEEL)) {
                short wheel_delta = (short)mouse.usButtonData;
                float scroll_delta = (float)wheel_delta / WHEEL_DELTA;

                if (mouse.usButtonFlags & RI_MOUSE_HWHEEL) {
                    u64 scroll_chars = 1;
                    SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &scroll_chars, 0);
                    scroll_delta *= scroll_chars;
                } else {
                    u64 scroll_lines = 3;
                    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scroll_lines, 0);
                    if (scroll_lines != WHEEL_PAGESCROLL) scroll_delta *= scroll_lines;

                    event.type = WINDOW_EVENT_MOUSE_WHEEL;
                    event.scroll_delta = (s32)scroll_delta;
                    array_add(w->events, event);
                }
            }
        }

        break;
    }
    case WM_MOUSEWHEEL: {
        event.type = WINDOW_EVENT_MOUSE_WHEEL;
        event.scroll_delta = GET_WHEEL_DELTA_WPARAM(wparam);
        array_add(w->events, event);
        break;
    }
    case WM_DROPFILES: {
        event.type = WINDOW_EVENT_FILE_DROP;

        HDROP hdrop      = (HDROP)wparam;
        u32   drop_count = DragQueryFileA(hdrop, 0xFFFFFFFF, null, 0);
        char *paths      = (char *)alloc(drop_count * MAX_PATH_LENGTH, __temporary_allocator);
        
        for (u32 i = 0; i < drop_count; ++i) {
            DragQueryFileA(hdrop, i, paths + (i * MAX_PATH_LENGTH), MAX_PATH_LENGTH);
        }

        event.file_drops = paths;
        event.file_drop_count = drop_count;
        
        DragFinish(hdrop);

        array_add(w->events, event);
        break;
    }
	case WM_QUIT:
	case WM_CLOSE: {
		event.type = WINDOW_EVENT_QUIT;
        array_add(w->events, event);
		return DefWindowProcA(hwnd, umsg, wparam, lparam);
	}
	default:
		return DefWindowProcA(hwnd, umsg, wparam, lparam);
	}

	return 0;
}

static RECT get_window_border_rect() {
	const u32 style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
	const u32 ex_style = WS_EX_APPWINDOW;

	RECT rect = {0};
	AdjustWindowRectEx(&rect, style, 0, ex_style);
    
	return rect;
}

Window *new_window(u16 width, u16 height, const char *name, s16 x, s16 y) {
    auto w    = New(Window);
	w->win32  = New(Win32_Window);
    w->handle = w->win32;

    array_realloc(w->events, 32); // preallocate some reasonable per frame event count
    
    auto win32 = w->win32;
	win32->class_name = "win32_window";

	WNDCLASSEX wclass = {0};
	wclass.cbSize        = sizeof(wclass);
	wclass.style         = CS_HREDRAW | CS_VREDRAW;
	wclass.lpfnWndProc   = win32_window_proc;
	wclass.hInstance     = (HINSTANCE)&__ImageBase;
	wclass.hIcon         = LoadIconA(NULL, IDI_APPLICATION);
	wclass.hCursor       = LoadCursorA(NULL, IDC_ARROW);
	wclass.lpszClassName = win32->class_name;

	win32->class_atom = RegisterClassExA(&wclass);
	win32->hinstance = wclass.hInstance;

	const RECT border_rect = get_window_border_rect();
	x += (s16)border_rect.left;
	//y += border_rect.top;
	width  += (u16)border_rect.right - (u16)border_rect.left;
	height += (u16)border_rect.bottom - (u16)border_rect.top;

    w->width  = width;
    w->height = height;
    
	win32->hwnd = CreateWindowExA(0, win32->class_name, name,
                                 WS_OVERLAPPEDWINDOW, x, y, width, height,
                                 NULL, NULL, win32->hinstance, NULL);
    
	if (!win32->hwnd) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to create window", GetLastError());
        return {};
    }

	win32->hdc = GetDC(win32->hwnd);
	if (!win32->hdc) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to get device context", GetLastError());
        return {};
    }

	SetProp(win32->hwnd, window_prop_name, w);
    
    RAWINPUTDEVICE rids[1];
    rids[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
    rids[0].usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
    rids[0].dwFlags     = 0;
    rids[0].hwndTarget  = win32->hwnd;
    
    if (RegisterRawInputDevices(rids, 1, sizeof(rids[0])) == FALSE) {
        log(LOG_IDENT_WIN32, LOG_ERROR, "[0x%X] Failed to register raw input devices", GetLastError());
    }
    
#if DEVELOPER
    DragAcceptFiles(win32->hwnd, TRUE);
#endif
    
	return w;
}

void destroy(Window *w) {
    auto win32 = w->win32;

    ReleaseDC(win32->hwnd, win32->hdc);
	DestroyWindow(win32->hwnd);
	UnregisterClassA(win32->class_name, win32->hinstance);
}

void poll_events(Window *w) {
    array_clear(w->events);
    
	MSG msg = { .hwnd = w->win32->hwnd };
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage (&msg);
	}
}

void close(Window *w) {
    auto win32 = w->win32;
	PostMessage(win32->hwnd, WM_CLOSE, 0, 0);
}

bool is_valid(Window *w) {
    auto win32 = w->win32;
	return IsWindow(win32->hwnd);
}

bool set_title(Window *w, const char *title) {
    auto win32 = w->win32;
    return SetWindowText(win32->hwnd, title);
}

void lock_cursor(Window *w, bool lock) {
    if (w->cursor_locked == lock) return;

	w->cursor_locked = lock;

    auto win32 = w->win32;
    auto input = get_input_table();
        
	if (lock) {
		RECT rect;
		if (GetWindowRect(win32->hwnd, &rect)) ClipCursor(&rect);
		ShowCursor(false);

        POINT point;
		point.x = input->cursor_x = w->width / 2;
		point.y = input->cursor_y = w->height / 2;

		ClientToScreen(win32->hwnd, &point);
		SetCursorPos(point.x, point.y);

        input->cursor_offset_x = 0;
        input->cursor_offset_y = 0;
	} else {
		ClipCursor(NULL);
		ShowCursor(true);
	}
}

void set_cursor(Window *w, s32 x, s32 y) {
    POINT point;
    point.x = x;
    point.y = y;

    ClientToScreen(w->win32->hwnd, &point);
    SetCursorPos(point.x, point.y);
}

void get_cursor(Window *w, s32 *x, s32 *y) {
    POINT p;
    GetCursorPos(&p); 
    ScreenToClient(w->win32->hwnd, &p);

    if (x) *x = p.x;
    if (y) *y = p.y;
}

Input_Table *get_input_table () { static Input_Table input; return &input; }

static u32      key_code_to_vkey_table      [KEY_COUNT] = {};
static u32      key_code_to_scan_code_table [KEY_COUNT] = {};
static Key_Code vkey_to_key_code_table      [512]       = {};

static bool __init_key_conversion_tables() {
    key_code_to_vkey_table[KEY_SPACE]         = VK_SPACE;
    key_code_to_vkey_table[KEY_APOSTROPHE]    = VK_OEM_7;
    key_code_to_vkey_table[KEY_COMMA]         = VK_OEM_COMMA;
    key_code_to_vkey_table[KEY_MINUS]         = VK_OEM_MINUS;
    key_code_to_vkey_table[KEY_PERIOD]        = VK_OEM_PERIOD;
    key_code_to_vkey_table[KEY_SLASH]         = VK_OEM_2;
    key_code_to_vkey_table[KEY_0]             = 0x30;
    key_code_to_vkey_table[KEY_1]             = 0x31;
    key_code_to_vkey_table[KEY_2]             = 0x32;
    key_code_to_vkey_table[KEY_3]             = 0x33;
    key_code_to_vkey_table[KEY_4]             = 0x34;
    key_code_to_vkey_table[KEY_5]             = 0x35;
    key_code_to_vkey_table[KEY_6]             = 0x36;
    key_code_to_vkey_table[KEY_7]             = 0x37;
    key_code_to_vkey_table[KEY_8]             = 0x38;
    key_code_to_vkey_table[KEY_9]             = 0x39;
    key_code_to_vkey_table[KEY_SEMICOLON]     = VK_OEM_1;
    key_code_to_vkey_table[KEY_EQUAL]         = VK_OEM_PLUS;
    key_code_to_vkey_table[KEY_A]             = 0x41;
    key_code_to_vkey_table[KEY_B]             = 0x42;
    key_code_to_vkey_table[KEY_C]             = 0x43;
    key_code_to_vkey_table[KEY_D]             = 0x44;
    key_code_to_vkey_table[KEY_E]             = 0x45;
    key_code_to_vkey_table[KEY_F]             = 0x46;
    key_code_to_vkey_table[KEY_G]             = 0x47;
    key_code_to_vkey_table[KEY_H]             = 0x48;
    key_code_to_vkey_table[KEY_I]             = 0x49;
    key_code_to_vkey_table[KEY_J]             = 0x4A;
    key_code_to_vkey_table[KEY_K]             = 0x4B;
    key_code_to_vkey_table[KEY_L]             = 0x4C;
    key_code_to_vkey_table[KEY_M]             = 0x4D;
    key_code_to_vkey_table[KEY_N]             = 0x4E;
    key_code_to_vkey_table[KEY_O]             = 0x4F;
    key_code_to_vkey_table[KEY_P]             = 0x50;
    key_code_to_vkey_table[KEY_Q]             = 0x51;
    key_code_to_vkey_table[KEY_R]             = 0x52;
    key_code_to_vkey_table[KEY_S]             = 0x53;
    key_code_to_vkey_table[KEY_T]             = 0x54;
    key_code_to_vkey_table[KEY_U]             = 0x55;
    key_code_to_vkey_table[KEY_V]             = 0x56;
    key_code_to_vkey_table[KEY_W]             = 0x57;
    key_code_to_vkey_table[KEY_X]             = 0x58;
    key_code_to_vkey_table[KEY_Y]             = 0x59;
    key_code_to_vkey_table[KEY_Z]             = 0x5A;
    key_code_to_vkey_table[KEY_LEFT_BRACKET]  = VK_OEM_4;
    key_code_to_vkey_table[KEY_BACKSLASH]     = VK_OEM_5;
	key_code_to_vkey_table[KEY_RIGHT_BRACKET] = VK_OEM_6;
    key_code_to_vkey_table[KEY_GRAVE_ACCENT]  = VK_OEM_3;
    key_code_to_vkey_table[KEY_ESCAPE]        = VK_ESCAPE;
    key_code_to_vkey_table[KEY_ENTER]         = VK_RETURN;
    key_code_to_vkey_table[KEY_TAB]           = VK_TAB;
    key_code_to_vkey_table[KEY_BACKSPACE]     = VK_BACK;
    key_code_to_vkey_table[KEY_INSERT]        = VK_INSERT;
    key_code_to_vkey_table[KEY_DELETE]        = VK_DELETE;
    key_code_to_vkey_table[KEY_RIGHT]         = VK_RIGHT;
    key_code_to_vkey_table[KEY_LEFT]          = VK_LEFT;
    key_code_to_vkey_table[KEY_DOWN]          = VK_DOWN;
    key_code_to_vkey_table[KEY_UP]            = VK_UP;
    key_code_to_vkey_table[KEY_PAGE_UP]       = VK_PRIOR;
    key_code_to_vkey_table[KEY_PAGE_DOWN]     = VK_NEXT;
    key_code_to_vkey_table[KEY_HOME]          = VK_HOME;
    key_code_to_vkey_table[KEY_END]           = VK_END;
    key_code_to_vkey_table[KEY_CAPS_LOCK]     = VK_CAPITAL;
    key_code_to_vkey_table[KEY_SCROLL_LOCK]   = VK_SCROLL;
    key_code_to_vkey_table[KEY_NUM_LOCK]      = VK_NUMLOCK;
    key_code_to_vkey_table[KEY_PRINT_SCREEN]  = VK_SNAPSHOT;
    key_code_to_vkey_table[KEY_PAUSE]         = VK_PAUSE;
    key_code_to_vkey_table[KEY_F1]            = VK_F1;
    key_code_to_vkey_table[KEY_F2]            = VK_F2;
    key_code_to_vkey_table[KEY_F3]            = VK_F3;
    key_code_to_vkey_table[KEY_F4]            = VK_F4;
    key_code_to_vkey_table[KEY_F5]            = VK_F5;
    key_code_to_vkey_table[KEY_F6]            = VK_F6;
    key_code_to_vkey_table[KEY_F7]            = VK_F7;
    key_code_to_vkey_table[KEY_F8]            = VK_F8;
    key_code_to_vkey_table[KEY_F9]            = VK_F9;
    key_code_to_vkey_table[KEY_F10]           = VK_F10;
    key_code_to_vkey_table[KEY_F11]           = VK_F11;
    key_code_to_vkey_table[KEY_F12]           = VK_F12;
    key_code_to_vkey_table[KEY_F13]           = VK_F13;
    key_code_to_vkey_table[KEY_F14]           = VK_F14;
    key_code_to_vkey_table[KEY_F15]           = VK_F15;
    key_code_to_vkey_table[KEY_F16]           = VK_F16;
    key_code_to_vkey_table[KEY_F17]           = VK_F17;
    key_code_to_vkey_table[KEY_F18]           = VK_F18;
    key_code_to_vkey_table[KEY_F19]           = VK_F19;
    key_code_to_vkey_table[KEY_F20]           = VK_F20;
    key_code_to_vkey_table[KEY_F21]           = VK_F21;
    key_code_to_vkey_table[KEY_F22]           = VK_F22;
    key_code_to_vkey_table[KEY_F23]           = VK_F23;
    key_code_to_vkey_table[KEY_F24]           = VK_F24;
    key_code_to_vkey_table[KEY_F25]           = KEY_NONE;
    key_code_to_vkey_table[KEY_KP_0]          = VK_NUMPAD0;
    key_code_to_vkey_table[KEY_KP_1]          = VK_NUMPAD1;
    key_code_to_vkey_table[KEY_KP_2]          = VK_NUMPAD2;
    key_code_to_vkey_table[KEY_KP_3]          = VK_NUMPAD3;
    key_code_to_vkey_table[KEY_KP_4]          = VK_NUMPAD4;
    key_code_to_vkey_table[KEY_KP_5]          = VK_NUMPAD5;
    key_code_to_vkey_table[KEY_KP_6]          = VK_NUMPAD6;
    key_code_to_vkey_table[KEY_KP_7]          = VK_NUMPAD7;
    key_code_to_vkey_table[KEY_KP_8]          = VK_NUMPAD8;
    key_code_to_vkey_table[KEY_KP_9]          = VK_NUMPAD9;
    key_code_to_vkey_table[KEY_KP_DECIMAL]    = VK_DECIMAL;
    key_code_to_vkey_table[KEY_KP_DIVIDE]     = VK_DIVIDE;
    key_code_to_vkey_table[KEY_KP_MULTIPLY]   = VK_MULTIPLY;
    key_code_to_vkey_table[KEY_KP_SUBTRACT]   = VK_SUBTRACT;
    key_code_to_vkey_table[KEY_KP_ADD]        = VK_ADD;
    key_code_to_vkey_table[KEY_KP_ENTER]      = KEY_NONE;
    key_code_to_vkey_table[KEY_KP_EQUAL]      = KEY_NONE;
    key_code_to_vkey_table[KEY_SHIFT]         = VK_SHIFT;
    key_code_to_vkey_table[KEY_CTRL]          = VK_CONTROL;
    key_code_to_vkey_table[KEY_ALT]           = VK_MENU;
    key_code_to_vkey_table[KEY_LEFT_SHIFT]    = VK_LSHIFT;
    key_code_to_vkey_table[KEY_LEFT_CTRL]     = VK_LCONTROL;
    key_code_to_vkey_table[KEY_LEFT_ALT]      = VK_LMENU;
    key_code_to_vkey_table[KEY_LEFT_SUPER]    = KEY_NONE;
    key_code_to_vkey_table[KEY_RIGHT_SHIFT]   = VK_RSHIFT;
    key_code_to_vkey_table[KEY_RIGHT_CTRL]    = VK_RCONTROL;
    key_code_to_vkey_table[KEY_RIGHT_ALT]     = VK_RMENU;
    key_code_to_vkey_table[KEY_RIGHT_SUPER]   = KEY_NONE;
    key_code_to_vkey_table[KEY_MENU]          = KEY_NONE;

	for (auto key = 0; key < KEY_COUNT; ++key) {
		const u16 vkey = key_code_to_vkey_table[key];
		if (vkey == 0) continue;

        vkey_to_key_code_table[vkey] = (Key_Code)key;

		const UINT scan_code = MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
		if (scan_code == 0) continue;

		key_code_to_scan_code_table[key] = scan_code;
	}
    
    return true;
}

static auto __init_key_conversion_tables_result = __init_key_conversion_tables();

Key_Code vkey_to_key_code(u32 vkey)     { return vkey_to_key_code_table[vkey]; }
u32      key_code_to_vkey(Key_Code key) { return key_code_to_vkey_table[key ]; }
