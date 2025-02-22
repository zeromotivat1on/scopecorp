#include "pch.h"
#include "render_registry.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "shader.h"
#include "texture.h"
#include <string.h>

void init_render_registry(Render_Registry* registry)
{
    registry->vertex_buffers = alloc_array_persistent(MAX_VERTEX_BUFFERS, Vertex_Buffer);
    registry->index_buffers  = alloc_array_persistent(MAX_INDEX_BUFFERS, Index_Buffer);
    registry->shaders        = alloc_array_persistent(MAX_SHADERS, Shader);
    registry->textures       = alloc_array_persistent(MAX_TEXTURES, Texture);
}

s32 add_vertex_buffer(Render_Registry* registry, Vertex_Buffer* buffer)
{
    if (registry->vertex_buffer_count >= MAX_VERTEX_BUFFERS) return INVALID_INDEX;
    const s32 idx = registry->vertex_buffer_count++;
    memcpy(registry->vertex_buffers + idx, buffer, sizeof(Vertex_Buffer));
    return idx;
}

s32 add_index_buffer(Render_Registry* registry, Index_Buffer* buffer)
{
    if (registry->index_buffer_count >= MAX_INDEX_BUFFERS) return INVALID_INDEX;
    const s32 idx = registry->index_buffer_count++;
    memcpy(registry->index_buffers + idx, buffer, sizeof(Index_Buffer));
    return idx;
}

s32 add_shader(Render_Registry* registry, Shader* shader)
{
    if (registry->shader_count >= MAX_SHADERS) return INVALID_INDEX;
    const s32 idx = registry->shader_count++;
    memcpy(registry->shaders + idx, shader, sizeof(Shader));
    return idx;
}

s32 add_texture(Render_Registry* registry, Texture* texture)
{
    if (registry->texture_count >= MAX_TEXTURES) return INVALID_INDEX;
    const s32 idx = registry->texture_count++;
    memcpy(registry->textures + idx, texture, sizeof(Texture));
    return idx;
}
