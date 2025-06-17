#pragma once

#include "sid.h"
#include "hash_table.h"
#include "sparse_array.h"

#define SID_MATERIAL_GEOMETRY   SID("/data/materials/geometry.mat")
#define SID_MATERIAL_OUTLINE    SID("/data/materials/outline.mat")
#define SID_MATERIAL_SKYBOX     SID("/data/materials/skybox.mat")
#define SID_MATERIAL_PLAYER     SID("/data/materials/player.mat")
#define SID_MATERIAL_GROUND     SID("/data/materials/ground.mat")
#define SID_MATERIAL_CUBE       SID("/data/materials/cube.mat")
#define SID_MATERIAL_UI_TEXT    SID("/data/materials/ui_text.mat")
#define SID_MATERIAL_UI_ELEMENT SID("/data/materials/ui_element.mat")

#define SID_MESH_PLAYER SID("/data/meshes/player.mesh")
#define SID_MESH_SKYBOX SID("/data/meshes/skybox.mesh")
#define SID_MESH_CUBE   SID("/data/meshes/cube.mesh")
#define SID_MESH_QUAD   SID("/data/meshes/quad.mesh")

#define SID_SOUND_WIND_AMBIENCE SID("/data/sounds/wind_ambience.wav")
#define SID_SOUND_PLAYER_STEPS  SID("/data/sounds/player_steps.wav")

#define SID_FONT_CONSOLA    SID("/data/fonts/consola.ttf")
#define SID_FONT_BETTER_VCR SID("/data/fonts/better_vcr.ttf")

#define SID_FLIP_BOOK_PLAYER_MOVE_LEFT    SID("/data/flip_books/player_move_left.fb")
#define SID_FLIP_BOOK_PLAYER_MOVE_RIGHT   SID("/data/flip_books/player_move_right.fb")
#define SID_FLIP_BOOK_PLAYER_MOVE_BACK    SID("/data/flip_books/player_move_back.fb")
#define SID_FLIP_BOOK_PLAYER_MOVE_FORWARD SID("/data/flip_books/player_move_forward.fb")

#define ASSET_PACK_EXTENSION_NAME ".pak"
#define GAME_ASSET_PACK_PATH   PATH_PACK(GAME_NAME ASSET_PACK_EXTENSION_NAME)
#define ASSET_PACK_MAGIC_VALUE U32_PACK('c', 'o', 'r', 'p')
#define ASSET_PACK_VERSION     0

inline constexpr s32 MAX_ASSET_SOURCES = 256;

inline constexpr s32 MAX_SHADER_INCLUDES = 16;
inline constexpr s32 MAX_SHADERS         = 64;
inline constexpr s32 MAX_TEXTURES        = 64;
inline constexpr s32 MAX_MATERIALS       = 64;
inline constexpr s32 MAX_MESHES          = 64;
inline constexpr s32 MAX_SOUNDS          = 64;
inline constexpr s32 MAX_FONTS           = 8;
inline constexpr s32 MAX_FLIP_BOOKS      = 64;

enum Asset_Type : u8 {
    ASSET_SHADER_INCLUDE,    
    ASSET_SHADER,
    ASSET_TEXTURE,
    ASSET_MATERIAL,
    ASSET_MESH,
    ASSET_SOUND,
    ASSET_FONT,
    ASSET_FLIP_BOOK,

    ASSET_TYPE_COUNT,
    
    ASSET_NONE = U8_MAX,
};

struct Asset {
    Asset_Type asset_type = ASSET_NONE;
    sid sid_path = SID_NONE; // sid to relative path ofc
    
    u64 data_offset = 0;
    u64 data_size   = 0;
    u64 path_offset = 0;
    u64 path_size   = 0; // includes null termination character
};

struct Asset_Source {
    Asset_Type asset_type = ASSET_NONE;
    u64 last_write_time = 0;
    sid sid_full_path = SID_NONE;     // e.g: "c:/game/run_tree/data/shaders/main.glsl"
    sid sid_relative_path = SID_NONE; // e.g: "/data/shaders/main.glsl"
};

// Structure of asset pack file is the following:
// - Header
// - Asset description data
// - Asset binary data

struct Asset_Pack_Header {
    u32 magic_value;
    u32 version;
    s32 count_by_type [ASSET_TYPE_COUNT];
    u64 offset_by_type[ASSET_TYPE_COUNT];
    u64 data_offset;
};

// Relative asset path sids are used to get either Asset_Source or specific Asset.

struct Asset_Source_Table {
    Hash_Table<sid, Asset_Source> table;
    s32 count_by_type[ASSET_TYPE_COUNT];
};

struct Asset_Table {
    Hash_Table<sid, struct Shader_Include> shader_includes;
    Hash_Table<sid, struct Shader>         shaders;
    Hash_Table<sid, struct Texture>        textures;
    Hash_Table<sid, struct Material>       materials;
    Hash_Table<sid, struct Mesh>           meshes;
    Hash_Table<sid, struct Sound>          sounds;
    Hash_Table<sid, struct Font>           fonts;
    Hash_Table<sid, struct Flip_Book>      flip_books;
};

inline Asset_Table asset_table;
inline Asset_Source_Table asset_source_table;

void init_asset_source_table();
void init_asset_table();

void save_asset_pack(const char *path);
void load_asset_pack(const char *path);

void convert_to_relative_asset_path(char *relative_path, const char *full_path);
void convert_to_full_asset_path(char *full_path, const char *relative_path);

u32 get_asset_type_size(Asset_Type type);
