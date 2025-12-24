#pragma once

#include "hash_table.h"
#include "file_system.h"

#define GAME_PAK_PATH    PATH_PAK(GAME_NAME ".pak")
#define GAME_PAK_MAGIC   U32_PACK('c', 'o', 'r', 'p')
#define GAME_PAK_VERSION 0

inline constexpr String GAME_PAK_META_ENTRY_NAME = S("_game_pak_meta_");

//
// Game pak specification.
// It uses pak file type under the hood, check pak.h for core specification.
//
// Meta entry is special, with reserved name, and contains special metadata for game pak.
//
// Meta entry
// - u32 : game pak magic
// - u32 : game pak version
// - u8  : count of next pairs
//   - u8  : asset type
//   - u16 : asset count
//   ...
//
// Assets are grouped and stored in file by their type.
// For now all assets are saved directly from disk files (no preprocessing to custom
// format before save), it may be changed later.
//

enum Asset_Type : u8 {
    ASSET_SHADER,
    ASSET_TEXTURE,
    ASSET_MATERIAL,
    ASSET_FLIP_BOOK,
    ASSET_MESH,
    ASSET_FONT,
    ASSET_SOUND,
};

bool save_game_pak (String path);
bool load_game_pak (String path);
