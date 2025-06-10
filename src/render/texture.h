#pragma once

#include "asset.h"

// Represents max texel count of texture width/height.
// E.g 4096 -> max texture that can be uploaded to GPU is 4096x4096.
inline s32 R_MAX_TEXTURE_TEXELS;

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);
inline constexpr s32 MAX_PLAYER_MOVE_FRAMES = 4;

typedef u64 sid;

enum Texture_Type : u8 {
    TEXTURE_TYPE_NONE,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_2D_ARRAY,
};

enum Texture_Format_Type : u8 {
    TEXTURE_FORMAT_NONE,
    TEXTURE_FORMAT_RED_INTEGER,
    TEXTURE_FORMAT_RGB_8,
    TEXTURE_FORMAT_RGBA_8,
    TEXTURE_FORMAT_DEPTH_24_STENCIL_8,
};

enum Texture_Wrap_Type : u8 {
    TEXTURE_WRAP_NONE,
    TEXTURE_WRAP_REPEAT,
	TEXTURE_WRAP_CLAMP,
	TEXTURE_WRAP_BORDER,
};

enum Texture_Filter_Type : u8 {
    TEXTURE_FILTER_NONE,
    TEXTURE_FILTER_LINEAR,
	TEXTURE_FILTER_NEAREST,
};

enum Texture_Flags : u32 {
    TEXTURE_FLAG_HAS_MIPMAPS = 0x1,
};

struct Texture : Asset {
    Texture() { asset_type = ASSET_TEXTURE; }
    
	rid rid = RID_NONE;
    u32 flags  = 0;
    s32 width  = 0;
	s32 height = 0;
	s32 channel_count = 0;
    Texture_Type        type   = TEXTURE_TYPE_NONE;
    Texture_Format_Type format = TEXTURE_FORMAT_NONE;
    Texture_Wrap_Type   wrap   = TEXTURE_WRAP_NONE;
    Texture_Filter_Type filter = TEXTURE_FILTER_NONE;
};

struct Texture_Sid_List {
	sid skybox;
	sid stone;
	sid grass;

	sid player_idle[DIRECTION_COUNT];
	sid player_move[DIRECTION_COUNT][4];
};

inline Texture_Sid_List texture_sids;

rid  r_create_texture(Texture_Type type, Texture_Format_Type format, s32 width, s32 height, void *data = null);
void r_delete_texture(rid rid_texture);
void r_set_texture_wrap(rid rid_texture, Texture_Wrap_Type wrap);
void r_set_texture_filter(rid rid_texture, Texture_Filter_Type filter, bool has_mipmaps = true);

void cache_texture_sids(Texture_Sid_List *list);
void init_texture_asset(Texture *texture, void *data);
void set_texture_wrap(Texture *texture, Texture_Wrap_Type wrap_type);
void set_texture_filter(Texture *texture, Texture_Filter_Type filter_type);
void generate_texture_mipmaps(Texture *texture);
void delete_texture(Texture *texture);

Texture_Format_Type get_texture_format_from_channel_count(s32 channel_count);
