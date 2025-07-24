#pragma once

#include "render/r_vertex.h"

struct R_Vertex_Binding {
    static constexpr u32 MAX_COMPONENTS = 8;
    
    s32 binding_index = INDEX_NONE;
    u32 offset = 0; // storage global offset
    
    u32 component_count = 0;
	Vertex_Component components[MAX_COMPONENTS];
};

struct R_Vertex_Descriptor {
    static constexpr u32 MAX_BINDINGS = 8;

    rid rid = RID_NONE;
    
    u32 binding_count = 0;
	R_Vertex_Binding bindings[MAX_BINDINGS] = { 0 };
};

// @Todo: hardcoded, make it more clear. Must be the same as in shaders.
// Check r_detect_capabilities for more info.
inline constexpr u32 EID_VERTEX_BINDING_INDEX = R_Vertex_Descriptor::MAX_BINDINGS;

u16 r_create_vertex_descriptor(u32 count, const R_Vertex_Binding *bindings);
