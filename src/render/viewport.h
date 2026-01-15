#pragma once

enum Viewport_Aspect_Type {
    VIEWPORT_FILL_WINDOW,
    VIEWPORT_4X3,
};

struct Viewport {
    Viewport_Aspect_Type aspect_type;

    Matrix4 transform = Matrix4_identity();
    
    u32 x = 0;
    u32 y = 0;
    u32 width  = 0;
    u32 height = 0;
    f32 resolution_scale = 1.0f;

    Vector2 cursor_pos = Vector2_zero;
    
    Matrix4 orthographic_projection;

    u32 framebuffer = 0;
    
    f32 pixel_size                  = 1.0f;
    f32 curve_distortion_factor     = 0.0f;
    f32 chromatic_aberration_offset = 0.0f;
    s32 quantize_color_count        = 256;
    f32 noise_blend_factor          = 0.0f;
    s32 scanline_count              = 0;
    f32 scanline_intensity          = 0.0f;
};

inline Viewport screen_viewport;

void init   (Viewport &viewport, u32 width, u32 height);
void resize (Viewport &viewport, u32 width, u32 height);
