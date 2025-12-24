#pragma once

#include "gpu.h"

struct Texture;
enum   Texture_Format_Type : u8;

inline constexpr u8 MAX_COLOR_ATTACHMENTS = 8;

struct Render_Target {
    Gpu_Handle handle = GPU_HANDLE_NONE;

    u16 width  = 0;
    u16 height = 0;

    Texture *color_attachments[MAX_COLOR_ATTACHMENTS];
    Texture *depth_attachment = null;

    u32 color_attachment_count = 0;
};

inline Render_Target screen_render_target = {};

Render_Target make_render_target  (u16 w, u16 h, const Texture_Format_Type *color_formats, u32 color_format_count, Texture_Format_Type depth_format);
void          resize              (Render_Target &target, u16 w, u16 h);
// @Todo: this reads pixel from currently bound render target, but as we now have command
// buffers that apply all backend stuff during flush, the function may not work as expected.
// Read pixel from most recent specified render target.
u32           read_pixel          (u32 color_attachment_index, u32 x, u32 y);
