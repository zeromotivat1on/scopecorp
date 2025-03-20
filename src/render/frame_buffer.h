#pragma once

inline constexpr s32 MAX_FRAME_BUFFER_COLOR_ATTACHMENTS = 4;

enum Texture_Format_Type;

// @Todo: support several color attachments.
struct Frame_Buffer {
    u32 id;
    s16 width;
    s16 height;

    Texture_Format_Type color_attachment_formats[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    s32 color_attachments[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    s32 color_attachment_count;

    Texture_Format_Type depth_attachment_format;
    s32            depth_attachment;
};

s32  create_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_attachment_formats, s32 color_attachment_count, Texture_Format_Type depth_attachment_format);
void recreate_frame_buffer(s32 fbi, s16 width, s16 height);
s32  read_frame_buffer_pixel(s32 x, s32 y); // read from currently used frame buffer

void start_frame_buffer_draw(s32 fbi);
void end_frame_buffer_draw();
void draw_frame_buffer(s32 fbi, s32 color_attachment_index);
