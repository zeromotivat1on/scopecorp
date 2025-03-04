#pragma once

#include "slot_array.h"
#include "render/vertex_buffer.h"
#include "render/index_buffer.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"

inline constexpr s32 MAX_VERTEX_BUFFERS = 64;
inline constexpr s32 MAX_INDEX_BUFFERS  = 64;
inline constexpr s32 MAX_SHADERS        = 64;
inline constexpr s32 MAX_TEXTURES       = 64;
inline constexpr s32 MAX_MATERIALS      = 64;

struct Render_Registry {
    Slot_Array<Vertex_Buffer> vertex_buffers;
    Slot_Array<Index_Buffer>  index_buffers;
    Slot_Array<Shader>        shaders;
    Slot_Array<Texture>       textures;
    Slot_Array<Material>      materials;
};

inline Render_Registry render_registry;

void init_render_registry(Render_Registry *registry);
