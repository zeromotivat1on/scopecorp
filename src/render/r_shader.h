#pragma once

#ifdef OPEN_GL
#define SHADER_FILE_EXT S(".glsl")
#else
#error "Unsupported graphics api"
#endif

enum Shader_Type {
    SHADER_VERTEX,
    SHADER_FRAGMENT,
    SHADER_COUNT
};

struct R_Shader {
    static constexpr u32 MAX_FILE_SIZE = KB(256);
    
    rid rid = RID_NONE;
};

void r_init_shader_compiler();
void r_destroy_shader_compiler();

u16  r_create_shader(String s);
void r_recreate_shader(u16 shader, String s);

String parse_shader_includes(Arena &a, String s);
String parse_shader_region(Arena &a, String s, Shader_Type type);
