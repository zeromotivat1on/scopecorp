#pragma once

inline constexpr s32 MAX_VERTEX_BUFFERS = 64;
inline constexpr s32 MAX_INDEX_BUFFERS  = 64;
inline constexpr s32 MAX_SHADERS        = 64;
inline constexpr s32 MAX_TEXTURES       = 64;

struct Render_Registry
{
    struct Vertex_Buffer* vertex_buffers;
    struct Index_Buffer*  index_buffers;
    struct Shader*        shaders;
    struct Texture*       textures;

    s32 vertex_buffer_count;
    s32 index_buffer_count;
    s32 shader_count;
    s32 texture_count;
};

inline Render_Registry render_registry;

void init_render_registry(Render_Registry* registry);
s32 add_vertex_buffer(Render_Registry* registry, Vertex_Buffer* buffer);
s32 add_index_buffer(Render_Registry* registry, Index_Buffer* buffer);
s32 add_shader(Render_Registry* registry, Shader* shader);
s32 add_texture(Render_Registry* registry, Texture* texture);
s32 recreate_shader(Render_Registry* registry, s32 shader_idx);
