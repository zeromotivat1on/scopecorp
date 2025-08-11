#pragma once

#include "math/matrix.h"

enum Viewport_Aspect_Type {
    VIEWPORT_FILL_WINDOW,
    VIEWPORT_4X3,
};

struct R_Viewport {
    Viewport_Aspect_Type aspect_type = VIEWPORT_FILL_WINDOW;
    
    u16 x = 0;
    u16 y = 0;
    u16 width  = 0;
    u16 height = 0;
    f32 resolution_scale = 1.0f;

    vec2 mouse_pos = vec2_zero;
    
    mat4 orthographic_projection;

    u16 render_target = 0;

    f32 pixel_size                  = 1.0f;
    f32 curve_distortion_factor     = 0.0f;
    f32 chromatic_aberration_offset = 0.0f;
    u32 quantize_color_count        = 256;
    f32 noise_blend_factor          = 0.0f;
    u32 scanline_count              = 0;
    f32 scanline_intensity          = 0.0f;
};

inline R_Viewport R_viewport;

void r_resize_viewport(R_Viewport &viewport, u16 width, u16 height);
void r_viewport_flush();
