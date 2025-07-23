#pragma once

// Represents max texel count of texture width/height.
// E.g 4096 -> max texture that can be uploaded to GPU is 4096x4096.
inline s32 R_MAX_TEXTURE_TEXELS;

/*
struct Texture_Address {
    u32 container = 0;
    f32 page = 0.0f;
};

struct Texture_Shape {
    Texture_Format_Type format = TEXTURE_FORMAT_NONE;
    u16 mipmap_count = 0;
    u32 width  = 0;
    u32 height = 0;
};
*/

struct R_Texture {
    static constexpr u32 MAX_FILE_SIZE = MB(4);

    // @Todo: use texture 'address' into global texture 2d array (per size) in shader.
    rid rid = RID_NONE;
    
    u16 width  = 0;
	u16 height = 0;
    
    u16 type   = 0;
    u16 format = 0;
    u16 wrap   = 0;
    
    u16 min_filter = 0;
    u16 mag_filter = 0;

    struct Meta {
        u16 width  = 0;
        u16 height = 0;
    
        u16 type   = 0;
        u16 format = 0;
        u16 wrap   = 0;
    
        u16 min_filter = 0;
        u16 mag_filter = 0;
    };
};

u16 r_create_texture(u16 type, u16 format, u16 w, u16 h, u16 wrap, u16 min_filter, u16 mag_filter, void *data = null);
void r_delete_texture(u16 texture);
u16 r_channel_count_from_format(u16 format);
u16 r_format_from_channel_count(u16 count);
