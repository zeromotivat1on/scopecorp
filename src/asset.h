#pragma once

#include "hash_table.h"

typedef void *File; // from os/file.h

#define SID_TEXTURE_PLAYER_IDLE_SOUTH SID("/data/textures/player_idle_back.png")
#define SID_TEXTURE_PLAYER_IDLE_EAST  SID("/data/textures/player_idle_right.png")
#define SID_TEXTURE_PLAYER_IDLE_WEST  SID("/data/textures/player_idle_left.png")
#define SID_TEXTURE_PLAYER_IDLE_NORTH SID("/data/textures/player_idle_forward.png")

inline sid sid_texture_player_idle[DIRECTION_COUNT];

#define SID_MATERIAL_GEOMETRY     SID("/data/materials/geometry.mat")
#define SID_MATERIAL_OUTLINE      SID("/data/materials/outline.mat")
#define SID_MATERIAL_SKYBOX       SID("/data/materials/skybox.mat")
#define SID_MATERIAL_PLAYER       SID("/data/materials/player.mat")
#define SID_MATERIAL_GROUND       SID("/data/materials/ground.mat")
#define SID_MATERIAL_CUBE         SID("/data/materials/cube.mat")
#define SID_MATERIAL_UI_TEXT      SID("/data/materials/ui_text.mat")
#define SID_MATERIAL_UI_ELEMENT   SID("/data/materials/ui_element.mat")
#define SID_MATERIAL_FRAME_BUFFER SID("/data/materials/frame_buffer.mat")

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

#define GAME_PAK_PATH PATH_PACK(GAME_NAME ".pak")
#define ASSET_PAK_MAGIC U32_PACK('c', 'o', 'r', 'p')
#define ASSET_PAK_VERSION 0

// @Todo: fix (de)serialization for: font.

enum Asset_Type : u8 {
    // @Note: order of enum declarations is crucial for asset pak (de)serialization.
    // Assets that reference other assets (e.g: material, flip book etc.) should appear
    // after more basic ones (e.g: shader, texture etc.).
    
    ASSET_SHADER,
    ASSET_TEXTURE,
    ASSET_MATERIAL,
    ASSET_MESH,
    ASSET_FONT,
    ASSET_FLIP_BOOK,
    ASSET_SOUND,

    ASSET_TYPE_COUNT,
    
    ASSET_NONE = U8_MAX,
};

struct Asset {
    Asset_Type type = ASSET_NONE;
    u16 index = 0; // to appropriate table with actual data
    
    sid path = SID_NONE;

    u32 pak_meta_size   = 0;
    u32 pak_data_size   = 0;
    u64 pak_blob_offset = 0;
};

struct Asset_Source {
    Asset_Type asset_type = ASSET_NONE;
    u64 last_write_time = 0;
    sid sid_full_path = SID_NONE;     // e.g: "c:/game/run_tree/data/shaders/main.glsl"
    sid sid_relative_path = SID_NONE; // e.g: "/data/shaders/main.glsl"
};

// Structure of asset pak file is the following:
// - Header
// - Asset description data
// - Asset binary data

struct Asset_Pak_Header {
    u32 magic   = 0;
    u32 version = 0;
    u32 counts [ASSET_TYPE_COUNT] = { 0 };
    u64 offsets[ASSET_TYPE_COUNT] = { 0 };
};

// Relative asset path sids are used to get either Asset_Source or specific Asset.

struct Asset_Source_Table {
    static constexpr u32 MAX_COUNT = 256;
    
    Table<sid, Asset_Source> table;
    u32 count_by_type[ASSET_TYPE_COUNT];
};

inline Asset_Source_Table Asset_source_table;

typedef Table<sid, Asset> Asset_Table;
inline Asset_Table Asset_table;

struct R_Shader;
struct R_Texture;
struct R_Material;
struct R_Mesh;

void init_asset_source_table();
void init_asset_table();

u32 get_asset_max_file_size(Asset_Type type);
u32 get_asset_meta_size(Asset_Type type);

Asset      *find_asset(sid path);
R_Shader   *find_shader(sid path);
R_Texture  *find_texture(sid path);
R_Material *find_material(sid path);
R_Mesh     *find_mesh(sid path);

void serialize  (File file, Asset &asset);
void deserialize(File file, Asset &asset);

void save_asset_pack(String path);
void load_asset_pack(String path);

String to_relative_asset_path(Arena &a, String path);
String to_full_asset_path(Arena &a, String path);
