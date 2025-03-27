#pragma once

#include "math/vector.h"

enum Vertex_Component_Type {
    VERTEX_S32,
    VERTEX_U32,
    VERTEX_F32,
    VERTEX_F32_2,
    VERTEX_F32_3,
    VERTEX_F32_4,
};

struct Vertex_PU {
    vec3 pos;
    vec2 uv;
};

struct Vertex_Entity {
    vec3 pos;
    vec2 uv;
    s32  entity_id;
};

s32 vertex_component_dimension(Vertex_Component_Type type);
s32 vertex_component_size(Vertex_Component_Type type);
