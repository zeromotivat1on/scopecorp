#include "pch.h"
#include "win32.h"
#include "render.h"
#include "window.h"
#include "glad.h"
#include "wglext.h"

#ifndef OPEN_GL
#error OpenGL implementation is included, but OPEN_GL macro is not defined
#endif

static constexpr auto LOG_IDENT_WGL = S("wgl");

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB = null;
PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT = null;

static Win32_Window wgl_create_dummy_window(Window *w) {
    auto win32 = w->win32;
    
	Win32_Window dummy_win32;
	dummy_win32.class_name = win32->class_name;
	dummy_win32.hinstance  = win32->hinstance;

	dummy_win32.hwnd = CreateWindowEx(0, dummy_win32.class_name, "dummy window",
                                      WS_OVERLAPPEDWINDOW, 0, 0, 1, 1,
                                      NULL, NULL, dummy_win32.hinstance, NULL);

	return dummy_win32;
}

static void wgl_create_dummy_context(Win32_Window *win32) {
	win32->hdc = GetDC(win32->hwnd);

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	const s32 pf = ChoosePixelFormat(win32->hdc, &pfd);
	if (pf == 0) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to choose dummy pixel format");
		return;
	}

	if (SetPixelFormat(win32->hdc, pf, &pfd) == FALSE) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to set dummy pixel format");
		return;
	}

	win32->hglrc = wglCreateContext(win32->hdc);
	if (wglMakeCurrent(win32->hdc, win32->hglrc) == FALSE) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to make dummy context current");
		return;
	}
}

static void wgl_destroy_dummy_window(Win32_Window *win32) {
	wglMakeCurrent(win32->hdc, NULL);
	wglDeleteContext(win32->hglrc);
	ReleaseDC(win32->hwnd, win32->hdc);
	DestroyWindow(win32->hwnd);

	win32 = {};
}

static s32 wgl_choose_pixel_format(Win32_Window *win32) {
	const s32 attributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 24,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB, GL_FALSE, // multisampling
		WGL_SAMPLES_ARB, 0,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_FALSE,
		0, 0
	};

	s32 pixel_format = 0;
	UINT num_formats = 0;
	BOOL status = wglChoosePixelFormatARB(win32->hdc, attributes, 0, 1, &pixel_format, &num_formats);

	if (status == FALSE || num_formats == 0) return 0;
	return pixel_format;
}

static void *wgl_get_proc_address(const char *name) {
	void *p = (void *)wglGetProcAddress(name);
	if (p == 0 || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1) {
		static HMODULE module = LoadLibraryA("opengl32.dll");
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

static inline const char *gl_debug_message_source_string(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:             return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
    case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
    case GL_DEBUG_SOURCE_OTHER:           return "Other";
    default:                              return "Invalid";
    }
}

static inline const char *gl_debug_message_type_string(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:               return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behaviour";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behaviour";
    case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
    case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
    case GL_DEBUG_TYPE_MARKER:              return "Marker";
    case GL_DEBUG_TYPE_PUSH_GROUP:          return "Push Group";
    case GL_DEBUG_TYPE_POP_GROUP:           return "Pop Group";
    case GL_DEBUG_TYPE_OTHER:               return "Other";
    default:                                return "Invalid";
    }
}

static inline const char *gl_debug_message_severity_string(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:         return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:       return "Medium";
    case GL_DEBUG_SEVERITY_LOW:          return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";
    default:                             return "Invalid";
    }
}

static inline Log_Level get_log_level_from_gl_debug_message_severity(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:         return LOG_MINIMAL;
    case GL_DEBUG_SEVERITY_MEDIUM:       return LOG_DEFAULT;
    case GL_DEBUG_SEVERITY_LOW:          return LOG_VERBOSE;
    case GL_DEBUG_SEVERITY_NOTIFICATION: return LOG_VERBOSE;
    default:                             return LOG_DEFAULT;
    }
}

static void gl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_data) {
    const char *source_string   = gl_debug_message_source_string(source);
    const char *type_string     = gl_debug_message_type_string(type);
    const char *severity_string = gl_debug_message_severity_string(severity);

	if (severity == GL_DEBUG_SEVERITY_HIGH) {
		volatile int a = 0;
	}

    const auto level = get_log_level_from_gl_debug_message_severity(severity);
    log(LOG_IDENT_GL, level, "[%d | %s | %s | %s] %.*s", id, source_string, type_string, severity_string, length, message);
}

bool init_render_backend(Window *w) {
    auto win32 = w->win32;
    
	Win32_Window dummy_win32 = wgl_create_dummy_window(w);
	wgl_create_dummy_context(&dummy_win32);

	if (dummy_win32.hglrc == NULL) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to create dummy OpenGL context");
		return false;
	}

	wgl_load_procs();

	const s32 pf = wgl_choose_pixel_format(&dummy_win32);
	if (pf == 0) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to choose pixel format");
		return false;
	}

	wgl_destroy_dummy_window(&dummy_win32);

	PIXELFORMATDESCRIPTOR pfd = {0};
	if (SetPixelFormat(win32->hdc, pf, &pfd) == FALSE) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to set pixel format");
		return false;
	}

	const s32 context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	win32->hglrc = wglCreateContextAttribsARB(win32->hdc, 0, context_attributes);
	if (win32->hglrc == NULL) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to create OpenGL context");
		return false;
	}

	if (wglMakeCurrent(win32->hdc, win32->hglrc) == FALSE) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to make context current");
		return false;
	}

	// @Cleanup: too lazy to load GL pointers manually,
	// maybe change and move glad source to own gl.h later.
	if (!gladLoadGLLoader((GLADloadproc)wgl_get_proc_address)) {
		log(LOG_IDENT_WGL, LOG_MINIMAL, "Failed to init GLAD");
		return false;
	}

    // Basic GL setup.

#if DEVELOPER
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
    glDebugMessageCallback(gl_debug_message_callback, null);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, null, GL_TRUE);
#endif
    
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &CBUFFER_OFFSET_ALIGNMENT);
        
	ShowWindow(win32->hwnd, SW_NORMAL);

    return true;
}

void set_vsync(Window *w, bool enable) {
    w->vsync = enable;
	wglSwapIntervalEXT(enable);
}

void swap_buffers(Window *w) {
    Profile_Zone(__func__);
    
    auto win32 = w->win32;
	SwapBuffers(win32->hdc);
}
