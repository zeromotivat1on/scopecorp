#pragma once

#include "asset.h"
#include "render/vertex.h"

inline constexpr u32 MAX_MESH_SIZE = MB(2);
inline constexpr s32 MAX_MESH_VERTEX_COMPONENTS = 8;

struct Mesh : Asset {
    Mesh() { asset_type = ASSET_MESH; }

    rid rid_vertex_array = RID_NONE;
    
    // Vertex data is stored in SOA way, for example:
    // vertex count = 3, layout description = p vec3, n vec3, uv vec2,
    // layout in memory = |ppp|nnn|uvuvuv|
    //
    // So, it's very important that .mesh specify vertex layout description
    // and vertex data in the same order.
    
    u32 vertex_count = 0;
    u32 vertex_data_offset = 0; // offset in global vertex buffer storage
    u32 vertex_data_size   = 0;

    u32 index_count = 0;
    u32 index_data_offset = 0;  // offset in global index buffer storage
    u32 index_data_size   = 0;

    // Vertex layout describes only components that vertex has. It does not describe
    // how vertex data is laid out in memory, see comment above for vertex data memory.
    Vertex_Component_Type vertex_layout[MAX_MESH_VERTEX_COMPONENTS];
    s32 vertex_layout_size = 0;
    s32 vertex_size = 0;
};

void init_mesh_asset(Mesh *mesh, void *data);
