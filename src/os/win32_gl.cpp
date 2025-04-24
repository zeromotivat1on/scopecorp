#include "pch.h"
#include "log.h"
#include "profile.h"

#include "render/glad.h"

#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "os/wglext.h"
#include "os/window.h"

#ifndef OPEN_GL
#error OpenGL implementation is included, but OPEN_GL macro is not defined
#endif

// Also defined in win32.cpp
struct Win32_Window {
	const char *class_name;
	ATOM class_atom;
	HINSTANCE hinstance;
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
};

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB = null;
PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT = null;

static Win32_Window wgl_create_dummy_window(Window *window) {
	Win32_Window win32;
	win32.class_name = window->win32->class_name;
	win32.hinstance = window->win32->hinstance;

	win32.hwnd = CreateWindowEx(0, win32.class_name, "dummy window",
		                        WS_OVERLAPPEDWINDOW, 0, 0, 1, 1,
		                        NULL, NULL, win32.hinstance, NULL);

	return win32;
}

static void wgl_create_dummy_context(Win32_Window *window) {
	window->hdc = GetDC(window->hwnd);

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	const s32 pf = ChoosePixelFormat(window->hdc, &pfd);
	if (pf == 0) {
		error("Failed to choose dummy pixel format");
		return;
	}

	if (SetPixelFormat(window->hdc, pf, &pfd) == FALSE) {
		error("Failed to set dummy pixel format");
		return;
	}

	window->hglrc = wglCreateContext(window->hdc);
	if (wglMakeCurrent(window->hdc, window->hglrc) == FALSE) {
		error("Failed to make dummy context current");
		return;
	}
}

static void wgl_destroy_dummy_window(Win32_Window *window) {
	wglMakeCurrent(window->hdc, NULL);
	wglDeleteContext(window->hglrc);
	ReleaseDC(window->hwnd, window->hdc);
	DestroyWindow(window->hwnd);

	*window = {0};
}

static s32 wgl_choose_pixel_format(Win32_Window *window) {
	const s32 attributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 24,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB, 1, // enable multisampling
		WGL_SAMPLES_ARB, 1,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		0, 0
	};

	s32 pixel_format = 0;
	UINT num_formats = 0;
	BOOL status = wglChoosePixelFormatARB(window->hdc, attributes, 0, 1, &pixel_format, &num_formats);

	if (status == FALSE || num_formats == 0) return 0;
	return pixel_format;
}

static void *wgl_get_proc_address(const char *name) {
	void *p = (void *)wglGetProcAddress(name);
	if (p == 0 || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1) {
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void *)GetProcAddress(module, name);
	}
	return p;
}

static void wgl_load_procs() {
#define load(name, type) name = (type)wgl_get_proc_address(#name)

	load(wglCreateContextAttribsARB, PFNWGLCREATECONTEXTATTRIBSARBPROC);
    load(wglChoosePixelFormatARB,    PFNWGLCHOOSEPIXELFORMATARBPROC);
    load(wglSwapIntervalEXT,         PFNWGLSWAPINTERVALEXTPROC);

#undef load
}

bool init_render_context(Window *window) {
    log("Platform: Windows | OpenGL");
    
	Win32_Window dummy_window = wgl_create_dummy_window(window);
	wgl_create_dummy_context(&dummy_window);

	if (dummy_window.hglrc == NULL) {
		error("Failed to create dummy opengl context");
		return false;
	}

	wgl_load_procs();

	const s32 pf = wgl_choose_pixel_format(&dummy_window);
	if (pf == 0) {
		error("Failed to choose pixel format");
		return false;
	}

	wgl_destroy_dummy_window(&dummy_window);

	PIXELFORMATDESCRIPTOR pfd = {0};
	if (SetPixelFormat(window->win32->hdc, pf, &pfd) == FALSE) {
		error("Failed to set pixel format");
		return false;
	}

	const s32 context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	window->win32->hglrc = wglCreateContextAttribsARB(window->win32->hdc, 0, context_attributes);
	if (window->win32->hglrc == NULL) {
		error("Failed to create opengl context");
		return false;
	}

	if (wglMakeCurrent(window->win32->hdc, window->win32->hglrc) == FALSE) {
		error("Failed to make context current");
		return false;
	}

	// @Cleanup: too lazy to load GL pointers manually,
	// maybe change and move glad source to own gl.h later.
	if (!gladLoadGLLoader((GLADloadproc)wgl_get_proc_address)) {
		error("Failed to init GLAD");
		return false;
	}

	ShowWindow(window->win32->hwnd, SW_NORMAL);
    return true;
}

void set_vsync(bool enable) {
	wglSwapIntervalEXT(enable);
}

void swap_buffers(Window *window) {
    PROFILE_SCOPE(__FUNCTION__);
	SwapBuffers(window->win32->hdc);
}
