#pragma once

#include "sparse_array.h"

#include "render/frame_buffer.h"
#include "render/vertex_buffer.h"
#include "render/index_buffer.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"

inline constexpr s32 MAX_FRAME_BUFFERS  = 4;
inline constexpr s32 MAX_VERTEX_BUFFERS = 64;
inline constexpr s32 MAX_INDEX_BUFFERS  = 64;
inline constexpr s32 MAX_SHADERS        = 64;
inline constexpr s32 MAX_TEXTURES       = 64;
inline constexpr s32 MAX_MATERIALS      = 64;
inline constexpr s32 MAX_UNIFORMS       = 64;

struct Render_Registry {
    Sparse_Array<Frame_Buffer>  frame_buffers;
    Sparse_Array<Vertex_Buffer> vertex_buffers;
    Sparse_Array<Index_Buffer>  index_buffers;
    Sparse_Array<Shader>        shaders;
    Sparse_Array<Texture>       textures;
    Sparse_Array<Material>      materials;
    Sparse_Array<Uniform>       uniforms;

    Uniform_Value_Cache uniform_value_cache;
};

inline Render_Registry render_registry;

void init_render_registry(Render_Registry *registry);
