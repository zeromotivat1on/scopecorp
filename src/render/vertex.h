#pragma once

#include "math/vector.h"

inline constexpr s32 MAX_VERTEX_LAYOUT_ATTRIBS = 4;

enum Vertex_Attrib_Type {
    VERTEX_ATTRIB_NULL,
    VERTEX_ATTRIB_F32_V2,
    VERTEX_ATTRIB_F32_V3,
};

struct Vertex_Layout {
    Vertex_Attrib_Type attribs[MAX_VERTEX_LAYOUT_ATTRIBS];
};

struct Vertex_PC {
    vec3 pos;
    vec3 col;
};

struct Vertex_PU {
    vec3 pos;
    vec2 uv;
};

s32 vertex_attrib_dimension(Vertex_Attrib_Type type);
s32 vertex_attrib_size(Vertex_Attrib_Type type);
