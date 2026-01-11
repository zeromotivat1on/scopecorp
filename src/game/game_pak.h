#pragma once

#define GAME_PAK_PATH PATH_PAK(GAME_NAME ".pak")

enum Asset_Type : u8 {
    ASSET_NONE,
    ASSET_SHADER,
    ASSET_TEXTURE,
    ASSET_MATERIAL,
    ASSET_FLIP_BOOK,
    ASSET_MESH,
    ASSET_FONT,
    ASSET_SOUND,
};

bool load_game_pak (String path);
