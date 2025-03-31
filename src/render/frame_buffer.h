#pragma once

inline constexpr s32 MAX_FRAME_BUFFER_COLOR_ATTACHMENTS = 4;

enum Texture_Format_Type;

struct Frame_Buffer {
    u32 id = 0;
    s32 material_index = INVALID_INDEX;
    s16 width  = 0;
    s16 height = 0;

    Texture_Format_Type color_attachment_formats[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    s32 color_attachments[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    s32 color_attachment_count = 0;

    Texture_Format_Type depth_attachment_format;
    s32 depth_attachment = INVALID_INDEX;
};

s32  create_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_attachment_formats, s32 color_attachment_count, Texture_Format_Type depth_attachment_format);
void recreate_frame_buffer(s32 fbi, s16 width, s16 height);
s32 read_frame_buffer_pixel(s32 fbi, s32 color_attachment_index, s32 x, s32 y);

void draw_frame_buffer(s32 fbi, s32 color_attachment_index);
