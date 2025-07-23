#pragma once

struct R_Attachment {
    u16 texture = 0;
    u16 format  = 0;
};

struct R_Target {
    static constexpr u8 MAX_COLOR_ATTACHMENTS = 8;
    
    rid rid = RID_NONE; // framebuffer handle (or can be smth else...?)

    u16 width  = 0;
    u16 height = 0;

    u16 color_attachment_count = 0;
    R_Attachment color_attachments[MAX_COLOR_ATTACHMENTS];
    
    R_Attachment depth_attachment;
};

u16  r_create_render_target(u16 w, u16 h, u16 count, const u16 *cformats, u16 dformat);
void r_resize_render_target(u16 target, u16 w, u16 h);
s32  r_read_render_target_pixel(u16 target, u32 color_attachment_index, u32 x, u32 y);

void r_submit(const R_Target &target);

// Read pixel from current bound render target.
s32 r_read_pixel(u32 color_attachment_index, u32 x, u32 y);
