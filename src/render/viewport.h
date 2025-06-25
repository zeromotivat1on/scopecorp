#pragma once

#include "render/frame_buffer.h"
#include "math/matrix.h"

enum Viewport_Aspect_Type {
    VIEWPORT_FILL_WINDOW,
    VIEWPORT_4X3,
};

struct Viewport {
    Viewport_Aspect_Type aspect_type = VIEWPORT_FILL_WINDOW;
    
    s16 x = 0;
    s16 y = 0;
    s16 width  = 0;
    s16 height = 0;
    f32 resolution_scale = 1.0f; // applied for frame buffer

    vec2 mouse_pos = vec2_zero;
    
    mat4 orthographic_projection;

    Frame_Buffer frame_buffer;
};

inline Viewport viewport;

void resize_viewport(Viewport *viewport, s16 width, s16 height);
