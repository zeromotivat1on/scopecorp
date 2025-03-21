#pragma once

struct Window;
struct vec3;

enum Gfx_Feature_Flags : u32 {
    GFX_FLAG_BLEND          = 0x1,
    GFX_FLAG_DEPTH          = 0x2,
    GFX_FLAG_CULL_BACK_FACE = 0x4,
    GFX_FLAG_WINDING_CCW    = 0x8,
    GFX_FLAG_SCISSOR        = 0x10,
    GFX_FLAG_STENCIL        = 0x20,
};

enum Clear_Flags : u32 {
    CLEAR_FLAG_COLOR   = 0x1,
    CLEAR_FLAG_DEPTH   = 0x2,
    CLEAR_FLAG_STENCIL = 0x4,
};

inline u32 gfx_features = 0;

void init_gfx(Window *window);
void set_vsync(bool enable);
void swap_buffers(Window *window);

void set_gfx_features(u32 flags);
void add_gfx_features(u32 flags);
void remove_gfx_features(u32 flags);
void clear_screen(vec3 color, u32 flags);
