#pragma once

#include "gpu.h"

inline constexpr u32 MAX_VERTEX_COMPONENTS = 8;
inline constexpr u32 MAX_VERTEX_BINDINGS   = 8;

// @Todo: hardcoded, make it more clear. Must be the same as in shaders.
// Check r_detect_capabilities for more info.
inline constexpr u32 EID_VERTEX_BINDING_INDEX = MAX_VERTEX_BINDINGS;

enum Vertex_Component_Type : u8 {
    VC_VOID,
    VC_S32,
    VC_U32,
    VC_F32,
    VC_F32_2,
    VC_F32_3,
    VC_F32_4,

    VC_COUNT
};

struct Vertex_Component {
    Vertex_Component_Type type;
    u16 advance_rate = 0;   // advance per vertex (0) or per N instance(s)
    bool normalize = false; // normalize to [-1, 1] range
};

struct Vertex_Binding {
    s32 binding_index = INDEX_NONE;
    u64 offset = 0;
    u32 component_count = 0;
    Vertex_Component components[MAX_VERTEX_COMPONENTS];
};

struct Vertex_Descriptor {
    Gpu_Handle handle = GPU_HANDLE_NONE;
    u32 binding_count = 0;
	Vertex_Binding bindings[MAX_VERTEX_BINDINGS];
};

Vertex_Descriptor make_vertex_descriptor (const Vertex_Binding *bindings, u32 binding_count);

u16 get_vertex_component_dimension (Vertex_Component_Type type);
u16 get_vertex_component_size      (Vertex_Component_Type type);
