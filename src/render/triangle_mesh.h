#pragma once

#include "hash_table.h"

enum Mesh_File_Format : u8 {
    MESH_FILE_FORMAT_NONE,
    MESH_FILE_FORMAT_OBJ,
};

struct Triangle_Shape {
    String name;
    
    u32 vertex_input;
    u32 vertex_count = 0;
    u32 first_index  = 0;
    u32 index_count  = 0;

    Material *material = null;
};

struct Triangle_Mesh {
    u64 *vertex_offsets; // per binding in vertex input
    u32 vertex_input;
    u64 index_offset;
    u32 vertex_count;
    u32 first_index;
    u32 index_count;
    // @Todo: not used for now.
    Array <Triangle_Shape> shapes;
};

struct Global_Meshes {
    Triangle_Mesh *missing = null;
};

inline Table <Atom, Triangle_Mesh> mesh_table;
inline Global_Meshes global_meshes;

Triangle_Mesh *new_mesh (String path);
Triangle_Mesh *new_mesh (Atom name, Buffer contents, Mesh_File_Format format);
Triangle_Mesh *get_mesh (Atom name);

inline Mesh_File_Format get_mesh_file_format(String path) {
    const auto ext = get_extension(path);
    if (ext == S("obj")) return MESH_FILE_FORMAT_OBJ;
    log(LOG_ERROR, "Failed to determine mesh file format from %S", path);
    return MESH_FILE_FORMAT_NONE;
}
