#pragma once

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);

enum Texture_Flags : u32 {
    TEXTURE_FLAG_2D_ARRAY = 0x1,
};

struct Texture {
    u32 id;
    u32 flags;
    s32 width;
    s32 height;
    s32 color_channel_count;
    const char* path;
};

struct Texture_Index_List {
    s32 skybox;
    s32 stone;
    s32 player_idle[DIRECTION_COUNT];
    s32 player_move[DIRECTION_COUNT][4];
};

inline Texture_Index_List texture_index_list;

void load_game_textures(Texture_Index_List* list);
s32 create_texture(const char* path);
