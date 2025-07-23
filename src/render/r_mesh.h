#pragma once

struct R_Mesh {
    static constexpr u32 MAX_FILE_SIZE = MB(8);

    // Vertex data is stored in SOA way, e.g:
    // vertex count = 3, layout description = p vec3, n vec3, t vec2,
    // layout in memory = |ppp|nnn|ttt|
    u16 vertex_descriptor = 0;
    u32 vertex_count = 0;
    
    u32 first_index = 0; // in index storage
    u32 index_count = 0;

    struct Meta {
        u16 vertex_size = 0;
        u16 vertex_component_count = 0;
        u32 vertex_count = 0;
        u32 index_count = 0;

        // After this meta goes actual asset pak data in same order as counts:
        // - vertex components
        // - vertices in SOA layout
        // - indices
    };
};

u16 r_create_mesh(u16 vertex_descriptor, u32 vertex_count, u32 first_index = 0, u32 index_count = 0);
