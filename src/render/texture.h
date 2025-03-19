#pragma once

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);

enum Texture_Flags : u32 {
    TEXTURE_TYPE_2D                   = 0x1,
    TEXTURE_TYPE_2D_ARRAY             = 0x2,
    
    TEXTURE_FORMAT_RGB_8              = 0x4,
    TEXTURE_FORMAT_RGBA_8             = 0x8,
    TEXTURE_FORMAT_DEPTH_24_STENCIL_8 = 0x10,
    
    TEXTURE_WRAP_REPEAT               = 0x20,
	TEXTURE_WRAP_CLAMP                = 0x40,
    
	TEXTURE_FILTER_LINEAR             = 0x80,
	TEXTURE_FILTER_NEAREST            = 0x100,

    TEXTURE_HAS_MIPMAPS               = 0x200,
};

struct Texture {
	u32 id;
	u32 flags;
	s32 width;
	s32 height;
	const char *path;
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
s32 create_texture(u32 flags, s32 width, s32 height, void *data);
//s32 create_texture(Texture_Type type, Texture_Format_Type format, Texture_Wrap_Type wrap, Texture_Filter_Type filter, s32 width, s32 height, void *data, u32 flags);
void delete_texture(s32 texture_index);
