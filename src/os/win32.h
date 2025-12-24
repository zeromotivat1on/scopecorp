#pragma once

#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

struct Win32_Window {
	const char *class_name;
	ATOM class_atom;
	HINSTANCE hinstance;
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
};
