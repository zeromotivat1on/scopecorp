#pragma once

enum Frame_Buffer_Attachment_Flags : u32 {
    FRAME_BUFFER_ATTACHMENT_FLAG_COLOR         = 0x1,
    FRAME_BUFFER_ATTACHMENT_FLAG_DEPTH_STENCIL = 0x2,
};

struct Frame_Buffer {
    u32 id;
    s16 width;
    s16 height;
    u32 attachment_flags = 0;
    s32 color_attachment = INVALID_INDEX;
    s32 depth_stencil_attachment = INVALID_INDEX;
};

s32  create_frame_buffer(s16 width, s16 height, u32 attachment_flags);
void recreate_frame_buffer(s32 frame_buffer_index, s16 width, s16 height);
