#pragma once

// Represents max texel count of texture width/height.
// E.g 4096 -> max texture that can be uploaded to GPU is 4096x4096.
inline s32 R_MAX_TEXTURE_SIZE;

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);
inline constexpr s32 MAX_PLAYER_MOVE_FRAMES = 4;

typedef u64 sid;

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
    TEXTURE_FLAG_HAS_MIPMAPS = 0x1,
};

struct Texture_Memory {
    void *data = null;
    s32   width  = 0;
    s32   height = 0;
    s32   channel_count = 0;
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
};

struct Texture_Sid_List {
	sid skybox;
	sid stone;
	sid grass;

	sid player_idle[DIRECTION_COUNT];
	sid player_move[DIRECTION_COUNT][4];
};

inline Texture_Sid_List texture_sids;

void cache_texture_sids(Texture_Sid_List *list);
s32  create_texture(Texture_Type texture_type, Texture_Format_Type format_type, s32 width, s32 height, void *data);
bool recreate_texture(s32 texture_index, Texture_Type texture_type, Texture_Format_Type format_type, s32 width, s32 height, void *data);
void set_texture_wrap(s32 texture_index, Texture_Wrap_Type wrap_type);
void set_texture_filter(s32 texture_index, Texture_Filter_Type filter_type);
void generate_texture_mipmaps(s32 texture_index);
void delete_texture(s32 texture_index);

Texture_Memory load_texture_memory(const char *path, bool log_error = true);
void free_texture_memory(Texture_Memory *memory);

Texture_Format_Type get_desired_texture_format(s32 channel_count);
