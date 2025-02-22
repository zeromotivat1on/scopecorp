#include "pch.h"
#include "texture.h"
#include "os/file.h"
#include "render/gl.h"
#include "render/render_registry.h"
#include "profile.h"
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

void load_game_textures(Texture_Index_List* list)
{
    list->skybox = create_texture(DIR_TEXTURES "skybox.png");
    list->stone  = create_texture(DIR_TEXTURES "stone.png");
    
    list->player_idle[BACK] = create_texture(DIR_TEXTURES "player_idle_back.png");
    list->player_idle[RIGHT] = create_texture(DIR_TEXTURES "player_idle_right.png");
    list->player_idle[LEFT] = create_texture(DIR_TEXTURES "player_idle_left.png");
    list->player_idle[FORWARD] = create_texture(DIR_TEXTURES "player_idle_forward.png");

    list->player_move[BACK][0] = create_texture(DIR_TEXTURES "player_move_back_1.png");
    list->player_move[BACK][1] = create_texture(DIR_TEXTURES "player_move_back_2.png");
    list->player_move[BACK][2] = create_texture(DIR_TEXTURES "player_move_back_3.png");
    list->player_move[BACK][3] = create_texture(DIR_TEXTURES "player_move_back_4.png");

    list->player_move[RIGHT][0] = create_texture(DIR_TEXTURES "player_move_right_1.png");
    list->player_move[RIGHT][1] = create_texture(DIR_TEXTURES "player_move_right_2.png");
    list->player_move[RIGHT][2] = create_texture(DIR_TEXTURES "player_move_right_3.png");
    list->player_move[RIGHT][3] = create_texture(DIR_TEXTURES "player_move_right_4.png");

    list->player_move[LEFT][0] = create_texture(DIR_TEXTURES "player_move_left_1.png");
    list->player_move[LEFT][1] = create_texture(DIR_TEXTURES "player_move_left_2.png");
    list->player_move[LEFT][2] = create_texture(DIR_TEXTURES "player_move_left_3.png");
    list->player_move[LEFT][3] = create_texture(DIR_TEXTURES "player_move_left_4.png");

    list->player_move[FORWARD][0] = create_texture(DIR_TEXTURES "player_move_forward_1.png");
    list->player_move[FORWARD][1] = create_texture(DIR_TEXTURES "player_move_forward_2.png");
    list->player_move[FORWARD][2] = create_texture(DIR_TEXTURES "player_move_forward_3.png");
    list->player_move[FORWARD][3] = create_texture(DIR_TEXTURES "player_move_forward_4.png");
}

s32 create_texture(const char* path)
{
    char timer_string[256];
    sprintf_s(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
    SCOPE_TIMER(timer_string);
    
    Texture texture = {0};
    texture.path = path;
    
    stbi_set_flip_vertically_on_load(true);

    u64 buffer_size = 0;
    u8* buffer = alloc_buffer_temp(MAX_TEXTURE_SIZE);
    if (!read_file(path, buffer, MAX_TEXTURE_SIZE, &buffer_size))
    {
        free_buffer_temp(MAX_TEXTURE_SIZE);
        stbi_set_flip_vertically_on_load(false);
        return INVALID_INDEX;
    }
    
    u8* data = stbi_load_from_memory(buffer, (s32)buffer_size, &texture.width, &texture.height, &texture.color_channel_count, 4);
    if (!data)
    {
        error("Failed to load texture %s, stbi reason %s", path, stbi_failure_reason());
        stbi_set_flip_vertically_on_load(false);
        return INVALID_INDEX;
    }
    
    stbi_set_flip_vertically_on_load(false);

    texture.id = gl_create_texture(data, texture.width, texture.height);
    free_buffer_temp(MAX_TEXTURE_SIZE);
    
    return add_texture(&render_registry, &texture);
}
