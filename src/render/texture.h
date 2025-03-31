#pragma once

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);

enum Texture_Type {
    TEXTURE_TYPE_NONE,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_2D_ARRAY,
};

enum Texture_Format_Type {
    TEXTURE_FORMAT_NONE,
    TEXTURE_FORMAT_RED_INTEGER,
    TEXTURE_FORMAT_RGB_8,
    TEXTURE_FORMAT_RGBA_8,
    TEXTURE_FORMAT_DEPTH_24_STENCIL_8,
};

enum Texture_Wrap_Type {
    TEXTURE_WRAP_NONE,
    TEXTURE_WRAP_REPEAT,
	TEXTURE_WRAP_CLAMP,
	TEXTURE_WRAP_BORDER,
};

enum Texture_Filter_Type {
    TEXTURE_FILTER_NONE,
    TEXTURE_FILTER_LINEAR,
	TEXTURE_FILTER_NEAREST,
};

enum Texture_Flags : u32 {
    TEXTURE_HAS_MIPMAPS = 0x1,
};

struct Texture {
	u32 id;
    s32 width;
	s32 height;
    u32 flags = 0;
    Texture_Type        type;
    Texture_Format_Type format;
    Texture_Wrap_Type   wrap;
    Texture_Filter_Type filter;
	const char *path = null;
};

struct Texture_Index_List {
	s32 skybox;
	s32 stone;
	s32 grass;

	s32 player_idle[DIRECTION_COUNT];
	s32 player_move[DIRECTION_COUNT][4];
};

inline Texture_Index_List texture_index_list;

void load_game_textures(Texture_Index_List *list);
s32 create_texture(const char *path);
s32 create_texture(Texture_Type type, Texture_Format_Type format, s32 width, s32 height, void *data);
void set_texture_wrap(s32 texture_index, Texture_Wrap_Type wrap);
void set_texture_filter(s32 texture_index, Texture_Filter_Type filter);
void generate_texture_mipmaps(s32 texture_index);
//s32 create_texture(Texture_Type type, Texture_Format_Type format, Texture_Wrap_Type wrap, Texture_Filter_Type filter, s32 width, s32 height, void *data, u32 flags);
void delete_texture(s32 texture_index);
