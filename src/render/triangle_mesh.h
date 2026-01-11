#pragma once

#include "hash_table.h"

struct Triangle_Shape {
    String name;
    
    u32 vertex_input;
    u32 vertex_count = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;

    Material *material = null;
};

struct Triangle_Mesh {
    String path;
    String name;

    u32 vertex_input  = 0;
    u64 *vertex_offsets; // per binding in vertex input
    u64 index_offset  = 0;

    u32 vertex_count = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;

    // @Todo: not used for now.
    Array <Triangle_Shape> shapes;
};

struct Global_Meshes {
    Triangle_Mesh *missing = null;
};

inline Table <String, Triangle_Mesh> mesh_table;
inline Global_Meshes global_meshes;

Triangle_Mesh *new_mesh (String path);
Triangle_Mesh *new_mesh (String path, Buffer contents);
Triangle_Mesh *get_mesh (String name);
