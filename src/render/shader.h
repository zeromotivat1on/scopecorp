#pragma once

#include "hash_table.h"
#include "gpu.h"

#define DEBUG_SHADER_REFLECTION 0

inline const auto SHADER_SOURCE_EXT = S(".sl");
inline const auto SHADER_HEADER_EXT = S(".slh");

#pragma once

#ifdef OPEN_GL
#define SHADER_FILE_EXT S(".glsl")
#else
#error "Unsupported graphics api"
#endif

namespace slang {
    struct IModule;
}

enum Shader_Type : u8 {
    SHADER_NONE,
    SHADER_VERTEX,
    SHADER_HULL,
    SHADER_DOMAIN,
    SHADER_GEOMETRY,
    SHADER_PIXEL,
    SHADER_COMPUTE,

    SHADER_TYPE_COUNT
};

enum Shader_File_Type : u8 {
    SHADER_FILE_TYPE_NONE,
    SHADER_HEADER,
    SHADER_SOURCE,
};

struct Shader_Entry_Point {
    Shader_Type type;
    String name;
};

struct Shader_File {
    String path;
    String name;

    Shader_Entry_Point *entry_points = null;
    u32 entry_point_count = 0;

    Shader_File_Type type;
    
    slang::IModule *module = null;
};

struct Compiled_Shader {
    const Shader_File *shader_file = null;
    Buffer *compiled_sources = null; // for each entry point in shader file
    Array <Gpu_Resource> resources;
};

struct Shader {    
    const Shader_File *shader_file = null;
    Handle linked_program = {};
    Table <String, Gpu_Resource> resource_table;    
};

struct Shader_Platform {
    Table <String, Shader_File>   shader_file_table;
    Table <String, struct Shader> shader_table;

    // For now all constant buffers live here, so it means each cbuffer name
    // should be unique across all shader codebase which seems kinda bad.
    Table <String, struct Constant_Buffer> constant_buffer_table;
};

struct Global_Shaders {
    Shader *missing = null;
};

inline Shader_Platform shader_platform;
inline Global_Shaders  global_shaders;

void init_shader_platform     ();
// Can be called multiple times to destroy old and create new session, useful for hot reload.
void init_slang_local_session ();

// Create shader from given path. This function is just a wrapper around other more
// precise and lower-level functions for shader creation. You can safely pass both
// shader source and shader header paths, it will distinguish them and create only
// shader file for header and return null, but both shader file and shader for source
// and return shader.
Shader         *new_shader          (String path);
Shader         *new_shader          (String path, String source);
Shader         *new_shader          (Shader_File &shader_file);
Shader         *new_shader          (const Compiled_Shader &compiled_shader);
Shader         *get_shader          (String name);
Shader_File    *new_shader_file     (String path);
Shader_File    *new_shader_file     (String path, String source);
Compiled_Shader compile_shader_file (Shader_File &shader_file);

bool is_shader_source_path (String path);
bool is_shader_header_path (String path);

bool is_valid (const Shader_File &shader_file);
bool is_valid (const Compiled_Shader &compiled_shader);
bool is_valid (const Shader &shader);
