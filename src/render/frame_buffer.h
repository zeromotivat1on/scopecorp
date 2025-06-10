#pragma once

inline constexpr s32 MAX_FRAME_BUFFER_COLOR_ATTACHMENTS = 4;

enum Texture_Format_Type : u8;

struct Frame_Buffer_Attachment {
    rid rid_texture = RID_NONE;
    Texture_Format_Type format;
};

struct Frame_Buffer {
    rid rid = RID_NONE;
    sid sid_material = SID_NONE;
    
    s16 width  = 0;
    s16 height = 0;

    s32 color_attachment_count = 0;
    Frame_Buffer_Attachment color_attachments[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    Frame_Buffer_Attachment depth_attachment;

    f32 pixel_size                  = 1.0f;
    f32 curve_distortion_factor     = 0.0f;
    f32 chromatic_aberration_offset = 0.0f;
    u32 quantize_color_count        = 64;
    f32 noise_blend_factor          = 0.0f;
    u32 scanline_count              = 0;
    f32 scanline_intensity          = 0.0f;
};

void r_init_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_format, s32 color_format_count, Texture_Format_Type depth_format, Frame_Buffer *frame_buffer);
void r_recreate_frame_buffer(Frame_Buffer *frame_buffer, s16 width, s16 height);
s32 r_read_frame_buffer_pixel(rid rid_frame_buffer, s32 color_attachment_index, s32 x, s32 y);

void draw_frame_buffer(Frame_Buffer *frame_buffer, s32 color_attachment_index);
