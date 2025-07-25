#include "pch.h"
#include "log.h"
#include "profile.h"

#include "os/file.h"
#include "os/atomic.h"
#include "os/input.h"
#include "os/memory.h"
#include "os/sync.h"
#include "os/thread.h"
#include "os/time.h"
#include "os/window.h"

#include "editor/editor.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <intrin.h>

// Also defined in render/win32_gl.cpp
struct Win32_Window {
	const char *class_name;
	ATOM class_atom;
	HINSTANCE hinstance;
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
};

static constexpr u32 INVALID_THREAD_RESULT = ((DWORD)-1);

const u32 WAIT_INFINITE = INFINITE;

const Thread THREAD_NONE          = NULL;
const s32 THREAD_CREATE_IMMEDIATE = 0;
const s32 THREAD_CREATE_SUSPENDED = CREATE_SUSPENDED;

const Mutex     MUTEX_NONE      = NULL;
const Semaphore SEMAPHORE_NONE  = NULL;
const u32 CRITICAL_SECTION_SIZE = sizeof(CRITICAL_SECTION);

const File INVALID_FILE           = INVALID_HANDLE_VALUE;
const u32  FILE_FLAG_READ         = GENERIC_READ;
const u32  FILE_FLAG_WRITE        = GENERIC_WRITE;
const u32  FILE_OPEN_NEW          = CREATE_NEW;
const u32  FILE_OPEN_EXISTING     = OPEN_EXISTING;
const u32  FILE_TRUNCATE_EXISTING = TRUNCATE_EXISTING;

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
static const char *window_prop_name = "win32_window";

File os_open_file(const char *path, s32 open_type, u32 access_flags, bool log_error) {
	HANDLE handle = CreateFile(path, access_flags, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, open_type, FILE_ATTRIBUTE_NORMAL, NULL);

    if (log_error && handle == INVALID_HANDLE_VALUE)
        error("Failed to open file %s, win32 error 0x%X", path, GetLastError());
    
    return handle;
}

bool os_close_file(File handle) {
	return CloseHandle(handle);
}

s64 os_file_size(File handle) {
	LARGE_INTEGER size;
	if (!GetFileSizeEx(handle, &size)) return -1;
	return size.QuadPart;
}

bool os_read_file(File handle, void *buffer, u64 size, u64 *bytes_read) {
	return ReadFile(handle, buffer, (DWORD)size, (LPDWORD)bytes_read, NULL);
}

bool os_write_file(File handle, const void *buffer, u64 size, u64 *bytes_written) {
    DWORD size_written;
	BOOL result = WriteFile(handle, buffer, (DWORD)size, &size_written, NULL);
    if (bytes_written) *bytes_written = size_written;
    return result;
}

bool os_set_file_ptr(File handle, s64 position) {
    LARGE_INTEGER move_distance;
    move_distance.QuadPart = position;
    return SetFilePointerEx(handle, move_distance, NULL, FILE_BEGIN);
}

void for_each_file(const char *directory, For_Each_File_Callback callback, void *user_data) {
    WIN32_FIND_DATA find_data;
    TCHAR file_path     [MAX_PATH + 0];
    TCHAR directory_mask[MAX_PATH + 3];
    
    PathCombine(directory_mask, directory, "*");
    
    HANDLE file = FindFirstFile(directory_mask, &find_data);
    if (file == INVALID_HANDLE_VALUE) {
        error("Failed to search directory %s", directory_mask);
        return;
    }

    File_Callback_Data callback_data;
    callback_data.user_data = user_data;
    
    do {
        const char *file_name = find_data.cFileName;
        PathCombine(file_path, directory, file_name);           

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) continue;
            for_each_file(file_path, callback);
        } else {
            callback_data.path = file_path;
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

void extract_file_from_path(char *path) {
    PathStripPath(path);
}

s64 os_file_ptr(File handle) {
    LARGE_INTEGER position      = {0};
    LARGE_INTEGER move_distance = {0};
    if (SetFilePointerEx(handle, move_distance, &position, FILE_CURRENT))
        return position.QuadPart;
    return INDEX_NONE;
}

void *os_vm_reserve(void *addr, u64 size) {
	Assert(size > 0);
	return VirtualAlloc(addr, size, MEM_RESERVE, PAGE_READWRITE);
}

void *os_vm_commit(void *vm, u64 size) {
	Assert(vm);
	Assert(size > 0);
	return VirtualAlloc(vm, size, MEM_COMMIT, PAGE_READWRITE);
}

bool os_vm_decommit(void *vm, u64 size) {
	Assert(vm);
	Assert(size > 0);
	return VirtualFree(vm, size, MEM_DECOMMIT);
}

bool os_vm_release(void *vm) {
	Assert(vm);
	return VirtualFree(vm, 0, MEM_RELEASE);
}

static BOOL win32_wait_res_check(void *handle, DWORD res) {
	switch (res) {
	case WAIT_OBJECT_0:
		return true;
	case WAIT_ABANDONED:
		warn("Object %p was not released before owning thread termination", handle);
		return false;
	case WAIT_TIMEOUT:
		log("Wait time for object %p elapsed", handle);
		return false;
	case WAIT_FAILED:
		error("Failed to wait for object %p with error code %u", handle, GetLastError());
		return false;
	default:
		error("Unknown wait result %u for object %p", res, handle);
		return false;
	}
}

u64 os_current_thread_id() {
	return GetCurrentThreadId();
}

void os_sleep_thread(u32 ms) {
	Sleep(ms);
}

bool os_thread_active(Thread handle) {
	DWORD exit_code;
	GetExitCodeThread(handle, &exit_code);
	return exit_code == STILL_ACTIVE;
}

Thread os_create_thread(Thread_Entry entry, s32 create_type, void *user_data) {
	return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)entry, user_data, create_type, NULL);
}

void os_resume_thread(Thread handle) {
	const DWORD res = ResumeThread(handle);
	Assert(res != INVALID_THREAD_RESULT);
}

void os_suspend_thread(Thread handle) {
	const DWORD res = SuspendThread(handle);
	Assert(res != INVALID_THREAD_RESULT);
}

void os_terminate_thread(Thread handle) {
	DWORD exit_code;
	GetExitCodeThread(handle, &exit_code);
	const BOOL res = TerminateThread(handle, exit_code);
	Assert(res);
}

Semaphore os_create_semaphore(s32 init_count, s32 max_count) {
	return CreateSemaphore(NULL, (LONG)init_count, (LONG)max_count, NULL);
}

bool os_release_semaphore(Semaphore handle, s32 count, s32 *prev_count) {
	return ReleaseSemaphore(handle, count, (LPLONG)prev_count);
}

bool os_wait_semaphore(Semaphore handle, u32 ms) {
	const DWORD res = WaitForSingleObjectEx(handle, ms, FALSE);
	return win32_wait_res_check(handle, res);
}

Mutex os_create_mutex(bool signaled) {
	return CreateMutex(NULL, (LONG)signaled, NULL);
}

bool os_release_mutex(Mutex handle) {
	return ReleaseMutex(handle);
}

bool os_wait_mutex(Mutex handle, u32 ms) {
	const DWORD res = WaitForSingleObjectEx(handle, ms, FALSE);
	return win32_wait_res_check(handle, res);
}

void os_init_cs(Critical_Section handle, u32 spin_count) {
	if (spin_count > 0)
		InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION)handle, spin_count);
	else
		InitializeCriticalSection((LPCRITICAL_SECTION)handle);
}

void os_enter_cs(Critical_Section handle) {
	EnterCriticalSection((LPCRITICAL_SECTION)handle);
}

bool os_try_enter_cs(Critical_Section handle) {
	return TryEnterCriticalSection((LPCRITICAL_SECTION)handle);
}

void os_leave_cs(Critical_Section handle) {
	LeaveCriticalSection((LPCRITICAL_SECTION)handle);
}

void os_delete_cs(Critical_Section handle) {
	DeleteCriticalSection((LPCRITICAL_SECTION)handle);
}

s32 atomic_swap(s32 *dst, s32 val) {
	return InterlockedExchange((LONG *)dst, val);
}

void *atomic_swap(void **dst, void *val) {
	return InterlockedExchangePointer(dst, val);
}

s32 atomic_cmp_swap(s32 *dst, s32 val, s32 cmp) {
	return InterlockedCompareExchange((LONG *)dst, val, cmp);
}

void *atomic_cmp_swap(void **dst, void *val, void *cmp) {
	return InterlockedCompareExchangePointer(dst, val, cmp);
}

s32 atomic_add(s32 *dst, s32 val) {
	return InterlockedAdd((LONG *)dst, val);
}

s32 atomic_increment(s32 *dst) {
	return InterlockedIncrement((LONG *)dst);
}

s32 atomic_decrement(s32 *dst) {
	return InterlockedDecrement((LONG *)dst);
}

s64 os_time_since_boot_ms() {
	return GetTickCount64();
}

s64 os_perf_counter() {
	LARGE_INTEGER counter;
	const BOOL res = QueryPerformanceCounter(&counter);
	Assert(res); // @Robustness: handle win32 failure
	return counter.QuadPart;
}

s64 os_perf_hz_s() {
	static u64 frequency64 = 0;

	if (frequency64 == 0) {
		LARGE_INTEGER frequency;
		const BOOL res = QueryPerformanceFrequency(&frequency);
		Assert(res);
		frequency64 = frequency.QuadPart;
	}

	return frequency64;
}

s64 os_perf_hz_ms() {
	static u64 frequency64 = 0;

	if (frequency64 == 0) {
		LARGE_INTEGER frequency;
		const BOOL res = QueryPerformanceFrequency(&frequency);
		Assert(res);
		frequency64 = frequency.QuadPart / 1000;
	}

	return frequency64;
}

static LRESULT CALLBACK win32_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	auto *window = (Window *)GetProp(hwnd, window_prop_name);
	if (window == null) return DefWindowProc(hwnd, umsg, wparam, lparam);

    auto &w = *window;
    
	Window_Event event = {};
    event.with_ctrl  = check(input_table.keys.buckets, KEY_CTRL);
    event.with_shift = check(input_table.keys.buckets, KEY_SHIFT);
    event.with_alt   = check(input_table.keys.buckets, KEY_ALT);
        
	switch (umsg) {
    case WM_SETFOCUS: {
        w.focused = true;
        w.cursor_locked = w.last_cursor_locked;
        break;
    }
    case WM_KILLFOCUS: {
        clear(input_table.keys.buckets, KEY_ALT);
        
        w.focused = false;
        w.last_cursor_locked = w.cursor_locked;
        w.cursor_locked = false;
        
        break;
    }
	case WM_SIZE: {
        event.type = WINDOW_EVENT_RESIZE;
        event.prev_width  = w.width;
        event.prev_height = w.height;
        
		w.width = LOWORD(lparam);
		w.height = HIWORD(lparam);

		s32 resized_event_index = INDEX_NONE;
		for (s32 i = 0; i < window_event_queue_size; ++i)
			if (window_event_queue[i].type == WINDOW_EVENT_RESIZE)
				resized_event_index = i;

		if (resized_event_index == INDEX_NONE) {
			Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
			window_event_queue[window_event_queue_size++] = event;
		} else {
			window_event_queue[resized_event_index] = event;
		}

		break;
	}
    case WM_SYSKEYDOWN:
	case WM_KEYDOWN: {
        event.type = WINDOW_EVENT_KEYBOARD;
        event.key_repeat  = lparam & 0x40000000;
        event.key_press = !event.key_repeat;
        event.key_code = input_table.key_codes[wparam];

        Assert(event.key_code > 0);
        set(input_table.keys.buckets, event.key_code);

        s32 key_down_event_index = INDEX_NONE;
		for (s32 i = 0; i < window_event_queue_size; ++i)
			if (event.type == WINDOW_EVENT_KEYBOARD && window_event_queue[i].key_code == event.key_code)
				key_down_event_index = i;

        if (key_down_event_index == INDEX_NONE) {
			Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
			window_event_queue[window_event_queue_size++] = event;
		} else {
			window_event_queue[key_down_event_index] = event;
		}

		break;
	}
    case WM_SYSKEYUP:
	case WM_KEYUP: {
		event.type = WINDOW_EVENT_KEYBOARD;
		event.key_release = true;
		event.key_code = input_table.key_codes[wparam];

		Assert(event.key_code > 0);
        clear(input_table.keys.buckets, event.key_code);

		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

		break;
	}
	case WM_CHAR: {
		event.type = WINDOW_EVENT_TEXT_INPUT;
		event.character = (u32)wparam;

		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP: {
		event.type = WINDOW_EVENT_MOUSE;
		event.key_press = umsg == WM_LBUTTONDOWN;
		event.key_code = MOUSE_LEFT;

        if (event.key_press) {
            set(input_table.keys.buckets, event.key_code);
        } else {
            clear(input_table.keys.buckets, event.key_code);
        }
        
		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

		break;
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP: {
		event.type = WINDOW_EVENT_MOUSE;
		event.key_press = umsg == WM_RBUTTONDOWN;
		event.key_code = MOUSE_RIGHT;

        if (event.key_press) {
            set(input_table.keys.buckets, event.key_code);
        } else {
            clear(input_table.keys.buckets, event.key_code);
        }
        
		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

		break;
	}
	case WM_MBUTTONUP:
	case WM_MBUTTONDOWN: {
		event.type = WINDOW_EVENT_MOUSE;
		event.key_press = umsg == WM_MBUTTONDOWN;
		event.key_code = MOUSE_MIDDLE;

        if (event.key_press) {
            set(input_table.keys.buckets, event.key_code);
        } else {
            clear(input_table.keys.buckets, event.key_code);
        }
        
		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

		break;
	}
	case WM_MOUSEMOVE: {
		input_table.mouse_x = GET_X_LPARAM(lparam);
		input_table.mouse_y = w.height - GET_Y_LPARAM(lparam) - 1;
		break;
	}
    case WM_MOUSEWHEEL: {
        event.type = WINDOW_EVENT_MOUSE;
        event.scroll_delta = GET_WHEEL_DELTA_WPARAM(wparam);
        
        Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
        window_event_queue[window_event_queue_size++] = event;
        
        break;
    }
    case WM_DROPFILES: {
        static char paths[MAX_WINDOW_DROP_COUNT * MAX_PATH_LENGTH];
        
        event.type = WINDOW_EVENT_FILE_DROP;

        HDROP hdrop = (HDROP)wparam;
        u32 drop_count = DragQueryFile(hdrop, 0xFFFFFFFF, null, 0);
        
        if (drop_count > MAX_WINDOW_DROP_COUNT) {
            warn("Dropped file count %u exceeded max value %u, some files will be ignored", drop_count, MAX_WINDOW_DROP_COUNT);
            drop_count = MAX_WINDOW_DROP_COUNT;
        }
        
        for (u32 i = 0; i < drop_count; ++i) {
            DragQueryFile(hdrop, i, paths + (i * MAX_PATH_LENGTH), MAX_PATH_LENGTH);
        }

        event.file_drops = paths;
        event.file_drop_count = drop_count;
        
        DragFinish(hdrop);

		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;

        break;
    }
	case WM_QUIT:
	case WM_CLOSE: {
		event.type = WINDOW_EVENT_QUIT;
		Assert(window_event_queue_size < MAX_WINDOW_EVENT_QUEUE_SIZE);
		window_event_queue[window_event_queue_size++] = event;
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	default:
		return DefWindowProc(hwnd, umsg, wparam, lparam);
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

bool os_create_window(u16 width, u16 height, const char *name, s16 x, s16 y, Window &w) {
    Assert(w.win32 == null);
    
	w.win32 = allocpn(Win32_Window, 1);

    auto &win32 = *w.win32;
	win32.class_name = "win32_window";

	WNDCLASSEX wclass = {0};
	wclass.cbSize = sizeof(wclass);
	wclass.style = CS_HREDRAW | CS_VREDRAW;
	wclass.lpfnWndProc = win32_window_proc;
	wclass.hInstance = win32.hinstance;
	wclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wclass.lpszClassName = win32.class_name;

	win32.class_atom = RegisterClassEx(&wclass);
	win32.hinstance = (HINSTANCE)&__ImageBase;

	const RECT border_rect = get_window_border_rect();
	x += (s16)border_rect.left;
	//y += border_rect.top;
	width  += (u16)border_rect.right - (u16)border_rect.left;
	height += (u16)border_rect.bottom - (u16)border_rect.top;

    w.width  = width;
    w.height = height;
    
	win32.hwnd = CreateWindowEx(0, win32.class_name, name,
                                WS_OVERLAPPEDWINDOW, x, y, width, height,
                                NULL, NULL, win32.hinstance, NULL);

	if (win32.hwnd == NULL) {
        error("Failed to create window, win32 error 0x%X", GetLastError());
        return false;
    }

	win32.hdc = GetDC(win32.hwnd);
	if (win32.hdc == NULL) {
        error("Failed to get device context, win32 error 0x%X", GetLastError());
        return false;
    }

	SetProp(win32.hwnd, window_prop_name, &w);

#if DEVELOPER
    DragAcceptFiles(win32.hwnd, TRUE);
#endif
    
	return true;
}

void os_set_window_user_data(Window &w, void *user_data) {
    w.user_data = user_data;
}

void os_register_window_callback(Window &w, Window_Event_Callback callback) {
	w.event_callback = callback;
}

void os_destroy_window(Window &w) {
    auto &win32 = *w.win32;

    ReleaseDC(win32.hwnd, win32.hdc);
	DestroyWindow(win32.hwnd);
	UnregisterClass(win32.class_name, win32.hinstance);
}

void os_poll_window_events(Window &w) {
    auto &win32 = *w.win32;

	input_table.mouse_offset_x = 0;
	input_table.mouse_offset_y = 0;
    
	MSG msg = {0};
	msg.hwnd = win32.hwnd;

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    {   // Update extra key states.
        const u256 changes = input_table.keys ^ input_table.keys_last;
        input_table.keys_down = changes & input_table.keys;
        input_table.keys_up   = changes & ~input_table.keys;
                        
        mem_copy(&input_table.keys_last, &input_table.keys, sizeof(input_table.keys));
    }
    
	input_table.mouse_offset_x = input_table.mouse_x - input_table.mouse_last_x;
	input_table.mouse_offset_y = input_table.mouse_y - input_table.mouse_last_y;

	if (w.cursor_locked) {
		POINT point;
		point.x = input_table.mouse_last_x = w.width / 2;
		point.y = input_table.mouse_last_y = w.height / 2;

		ClientToScreen(win32.hwnd, &point);
		SetCursorPos(point.x, point.y);
	} else {
		input_table.mouse_last_x = input_table.mouse_x;
		input_table.mouse_last_y = input_table.mouse_y;
	}

	for (s32 i = 0; i < window_event_queue_size; ++i) {
		w.event_callback(w, window_event_queue[i]);
    }
}

void os_close_window(Window &w) {
    auto &win32 = *w.win32;
	PostMessage(win32.hwnd, WM_CLOSE, 0, 0);
}

bool os_window_alive(Window &w) {
    auto &win32 = *w.win32;
	return IsWindow(win32.hwnd);
}

bool os_set_window_title(Window &w, const char *title) {
    auto &win32 = *w.win32;
    return SetWindowText(win32.hwnd, title);
}

void os_lock_window_cursor(Window &w, bool lock) {
    auto &win32 = *w.win32;
    if (w.cursor_locked == lock) return;
    
	w.cursor_locked = lock;

	if (lock) {
		RECT rect;
		if (GetWindowRect(win32.hwnd, &rect)) ClipCursor(&rect);
		ShowCursor(false);

        POINT point;
		point.x = input_table.mouse_x = input_table.mouse_last_x = w.width / 2;
		point.y = input_table.mouse_y = input_table.mouse_last_y = w.height / 2;

		ClientToScreen(win32.hwnd, &point);
		SetCursorPos(point.x, point.y);

        input_table.mouse_offset_x = 0;
        input_table.mouse_offset_y = 0;
	} else {
		ClipCursor(NULL);
		ShowCursor(true);
	}
}

void init_input_table() {
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
    input_table.virtual_keys[KEY_F25]           = KEY_NONE;
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
    input_table.virtual_keys[KEY_KP_ENTER]      = KEY_NONE;
    input_table.virtual_keys[KEY_KP_EQUAL]      = KEY_NONE;
    input_table.virtual_keys[KEY_SHIFT]         = VK_SHIFT;
    input_table.virtual_keys[KEY_CTRL]          = VK_CONTROL;
    input_table.virtual_keys[KEY_ALT]           = VK_MENU;
    input_table.virtual_keys[KEY_LEFT_SHIFT]    = VK_LSHIFT;
    input_table.virtual_keys[KEY_LEFT_CTRL]     = VK_LCONTROL;
    input_table.virtual_keys[KEY_LEFT_ALT]      = VK_LMENU;
    input_table.virtual_keys[KEY_LEFT_SUPER]    = KEY_NONE;
    input_table.virtual_keys[KEY_RIGHT_SHIFT]   = VK_RSHIFT;
    input_table.virtual_keys[KEY_RIGHT_CTRL]    = VK_RCONTROL;
    input_table.virtual_keys[KEY_RIGHT_ALT]     = VK_RMENU;
    input_table.virtual_keys[KEY_RIGHT_SUPER]   = KEY_NONE;
    input_table.virtual_keys[KEY_MENU]          = KEY_NONE;

	for (u8 key = 0; key < KEY_COUNT; ++key) {
		const u16 virtual_key = input_table.virtual_keys[key];
		if (virtual_key == KEY_NONE) continue;

		input_table.key_codes[virtual_key] = key;

		const UINT scan_code = MapVirtualKey(virtual_key, MAPVK_VK_TO_VSC);
		if (scan_code == 0) continue;

		input_table.scan_codes[key] = scan_code;
	}
}
