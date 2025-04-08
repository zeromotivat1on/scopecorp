#pragma once

#include "sid.h"
#include "hash_table.h"
#include "sparse_array.h"

inline constexpr s32 MAX_ASSETS = 256;
inline constexpr s32 MAX_ASSET_NAME_SIZE = 64;

#define ASSET_PACK_MAGIC_VALUE U32_PACK('c', 'o', 'r', 'p')
#define ASSET_PACK_VERSION     0

enum Asset_Type {
    ASSET_NONE,
    ASSET_SHADER,
    ASSET_TEXTURE,
    ASSET_SOUND,
};

struct Asset_Source {
    Asset_Type type = ASSET_NONE;
    char path[MAX_PATH_SIZE] = {0};
};

struct Asset_Texture {
    s32 width  = 0;
    s32 height = 0;
    s32 channel_count = 0;
};

struct Asset_Shader {
    u32 size = 0;
};

struct Asset_Sound {
    u32 size = 0;
    s32 sample_rate   = 0;
    s32 channel_count = 0;
	s32 bit_depth     = 0;
};

struct Asset {
    Asset_Type type = ASSET_NONE;
    u64 data_offset = 0;
    s32 registry_index = INVALID_INDEX; // index to appropriate registry (audio, render etc.)
    char relative_path[MAX_PATH_SIZE] = {0};

    union {
        Asset_Shader  as_shader;
        Asset_Texture as_texture;
        Asset_Sound   as_sound;
    };

    Asset() : as_sound() {}
};

// Structure of asset pack file is the following:
// - Header
// - Asset description data
// - Asset extracted binary data

struct Asset_Pack_Header {
    u32 magic_value;
    u32 version;
    u32 asset_count;
};

typedef Hash_Table<sid, Asset>     Asset_Table;
typedef Sparse_Array<Asset_Source> Asset_Source_List;

inline Asset_Table       asset_table;
inline Asset_Source_List asset_sources;

void init_asset_sources(Asset_Source_List *sources);
void init_asset_table(Asset_Table *table);

void save_asset_pack(const char *path);
void load_asset_pack(const char *path, Asset_Table *table);

void convert_to_relative_asset_path(char *path);
void convert_to_full_asset_path(const char *relative_path, char *full_path);
