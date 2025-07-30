#pragma once

#ifdef OPEN_GL
#define SHADER_FILE_EXT S(".glsl")
#else
#error "Unsupported graphics api"
#endif

struct R_Shader {
    static constexpr u32 MAX_FILE_SIZE = KB(256);
    
    rid rid = RID_NONE;
};

u16  r_create_shader(String s);
void r_recreate_shader(u16 shader, String s);

String parse_shader_includes(Arena &a, String s);
bool parse_shader_regions(const char *source, char *out_vertex, char *out_fragment);
