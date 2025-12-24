#pragma once

#include "hash_table.h"
#include "vertex_descriptor.h"

struct Triangle_Shape {
    String name;
    
    Vertex_Descriptor vertex_descriptor;
    u32 vertex_count = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;

    Material *material = null;
};

struct Triangle_Mesh {
    String path;
    String name;

    Vertex_Descriptor vertex_descriptor;
    u32 vertex_count = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;

    // @Todo: not used for now.
    Array <Triangle_Shape> shapes;
};

inline Table <String, Triangle_Mesh> triangle_mesh_table;

Triangle_Mesh *new_mesh (String path);
Triangle_Mesh *new_mesh (String path, Buffer contents);
Triangle_Mesh *get_mesh (String name);
