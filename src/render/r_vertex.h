#pragma once

struct Vertex_Component {
    u16 type = 0;
    u16 advance_rate = 0;   // advance per vertex (0) or per N instance(s)
    bool normalize = false; // normalize to [-1, 1] range
};

inline constexpr Vertex_Component vertex_entity_layout[4] = {
    { R_F32_3, 0 },
    { R_F32_3, 0 },
    { R_F32_2, 0 },
    { R_S32,   1 },
};

u16 r_vertex_component_dimension(u16 type);
u16 r_vertex_component_size(u16 size);
