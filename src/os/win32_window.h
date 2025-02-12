#pragma once

#include "window.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Win32_Window
{
    const char* class_name;
    ATOM class_atom;
    HINSTANCE hinstance;
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
};
