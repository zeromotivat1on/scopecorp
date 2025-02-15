#include "pch.h"
#include "texture.h"
#include "file.h"
#include "render/gl.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

void load_game_textures(Texture_List* list)
{
    list->player = create_texture(DIR_TEXTURES "player.png");
    list->skybox = create_texture(DIR_TEXTURES "skybox.png");
}

Texture create_texture(const char* path)
{
    Texture texture = {0};
    texture.path = path;
    
    stbi_set_flip_vertically_on_load(true);

    u8* buffer = alloc_buffer_temp(MAX_TEXTURE_SIZE);
    // @Cleanup: use read file temp overload.
    if (!read_file(path, buffer, MAX_TEXTURE_SIZE, null))
    {
        stbi_set_flip_vertically_on_load(false);
        return {0};
    }
    
    u8* data = stbi_load_from_memory(buffer, MAX_TEXTURE_SIZE, &texture.width, &texture.height, &texture.color_count, 4);
    if (!data)
    {
        log("Failed to load texture (%s), stbi reason (%s)", path, stbi_failure_reason());
        stbi_set_flip_vertically_on_load(false);
        return {0};
    }
    
    stbi_set_flip_vertically_on_load(false);

    texture.id = gl_create_texture(data, texture.width, texture.height);
    free_buffer_temp(MAX_TEXTURE_SIZE);
    
    return texture;
}
