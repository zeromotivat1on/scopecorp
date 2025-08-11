#pragma once

#include "sparse.h"

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
    static constexpr u32 MAX_UNIFORM_VALUE_CACHE_SIZE = KB(16);
    
    static constexpr u16 MAX_TARGETS   = 16;
    static constexpr u16 MAX_PASSES    = 32;
    static constexpr u16 MAX_TEXTURES  = 64;
    static constexpr u16 MAX_SHADERS   = 64;
    static constexpr u16 MAX_UNIFORMS  = 256;
    static constexpr u16 MAX_MATERIALS = 64;
    static constexpr u16 MAX_MESHES    = 128;
    static constexpr u16 MAX_VERTEX_DESCRIPTORS = 128;
    static constexpr u16 MAX_FLIP_BOOKS = 64;

    Arena arena;
    
    Sparse<R_Target>   targets;
    Sparse<R_Pass>     passes;
    Sparse<R_Texture>  textures;
    Sparse<R_Shader>   shaders;
    Sparse<R_Uniform>  uniforms;
    Sparse<R_Material> materials;
    Sparse<R_Mesh>     meshes;
    Sparse<R_Vertex_Descriptor> vertex_descriptors;

    Sparse<R_Flip_Book> flip_books;

    struct { void *data = null; u32 size = 0; u32 capacity = 0; } uniform_value_cache;
};

inline R_Table R_table;

void r_create_table (R_Table &t);
void r_destroy_table(R_Table &t);
