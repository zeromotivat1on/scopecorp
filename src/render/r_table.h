#pragma once

#include "sparse_array.h"

struct R_Target;
struct R_Pass;
struct R_Texture;
struct R_Shader;
struct R_Uniform;
struct R_Material;
struct R_Mesh;
struct R_Vertex_Descriptor;
struct R_Flip_Book;

// Table of all currently active render resources.
struct R_Table {
    static constexpr u16 MAX_TARGETS   = 16;
    static constexpr u16 MAX_PASSES    = 32;
    static constexpr u16 MAX_TEXTURES  = 64;
    static constexpr u16 MAX_SHADERS   = 64;
    static constexpr u16 MAX_UNIFORMS  = 256;
    static constexpr u16 MAX_MATERIALS = 64;
    static constexpr u16 MAX_MESHES    = 128;
    static constexpr u16 MAX_VERTEX_DESCRIPTORS = 128;
    static constexpr u16 MAX_FLIP_BOOKS = 64;

    Sparse_Array<R_Target>   targets;
    Sparse_Array<R_Pass>     passes;
    Sparse_Array<R_Texture>  textures;
    Sparse_Array<R_Shader>   shaders;
    Sparse_Array<R_Uniform>  uniforms;
    Sparse_Array<R_Material> materials;
    Sparse_Array<R_Mesh>     meshes;
    Sparse_Array<R_Vertex_Descriptor> vertex_descriptors;

    Sparse_Array<R_Flip_Book> flip_books;
};

inline R_Table R_table;

void r_create_table(R_Table &t);
