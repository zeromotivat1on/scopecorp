#pragma once

inline constexpr s32 MAX_TEXTURE_SIZE = KB(256);

struct Texture
{
    u32 id;
    s32 width;
    s32 height;
    s32 color_channel_count;
    const char* path;
};

struct Texture_List
{
    Texture skybox;
    Texture stone;
    Texture player_idle[DIRECTION_COUNT];
    Texture player_move[DIRECTION_COUNT][4];
};

inline Texture_List textures;

void load_game_textures(Texture_List* list);
Texture create_texture(const char* path);
