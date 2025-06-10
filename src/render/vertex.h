#pragma once

#include "math/vector.h"

enum Vertex_Component_Type : u8 {
    VERTEX_S32,
    VERTEX_U32,
    VERTEX_F32,
    VERTEX_F32_2,
    VERTEX_F32_3,
    VERTEX_F32_4,
};

struct Vertex_Component {
    Vertex_Component_Type type;
    u8 advance_rate = 0;    // advance per vertex or per n instance
    bool normalize = false; // normalize to [-1, 1] range
};

struct Vertex_PU {
    vec3 pos;
    vec2 uv;
};

struct Vertex_Entity {
    vec3 pos;
    vec3 normal;
    vec2 uv;
    s32  entity_id;
};

inline constexpr Vertex_Component vertex_entity_layout[4] = {
    { VERTEX_F32_3, 0 },
    { VERTEX_F32_3, 0 },
    { VERTEX_F32_2, 0 },
    { VERTEX_S32,   1 },
};

s32 get_vertex_component_dimension(Vertex_Component_Type type);
s32 get_vertex_component_size(Vertex_Component_Type type);
