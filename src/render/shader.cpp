#include "pch.h"
#include "render/r_shader.h"

#include "log.h"
#include "str.h"
#include "asset.h"

#include "os/file.h"

enum Shader_Region_Type {
    REGION_VERTEX,
    REGION_FRAGMENT,
    REGION_COUNT
};

static const char *DECL_BEGIN_VERTEX = "#begin vertex";
static const char *DECL_END_VERTEX   = "#end vertex";

static const char *DECL_BEGIN_FRAGMENT = "#begin fragment";
static const char *DECL_END_FRAGMENT   = "#end fragment";

static inline const char *get_shader_region_begin_decl(Shader_Region_Type type) {
    switch (type) {
    case REGION_VERTEX:   return DECL_BEGIN_VERTEX;
    case REGION_FRAGMENT: return DECL_BEGIN_FRAGMENT;
    default:
        error("Unknown shader region type %d", type);
        return null;
    }
}

static inline const char *get_shader_region_end_decl(Shader_Region_Type type) {
    switch (type) {
    case REGION_VERTEX:   return DECL_END_VERTEX;
    case REGION_FRAGMENT: return DECL_END_FRAGMENT;
    default:
        error("Unknown shader region type %d", type);
        return null;
    }
}

static bool parse_shader_region(const char *in, char *out, Shader_Region_Type type) {
    Assert(type < REGION_COUNT);
    
    const char *decl_begin = get_shader_region_begin_decl(type);
    const char *decl_end   = get_shader_region_end_decl(type);

    const char *cin   = in;
    const char *begin = str_sub(cin, decl_begin);
    const char *end   = str_sub(cin, decl_end);
    
    if (begin == null) {
        error("Failed to find shader region begin declaration '%s'", decl_begin);
        return false;
    }

    if (end == null) {
        error("Failed to find shader region end declaration '%s'", decl_end);
        return false;
    }

    *out = '\0';

    cin = str_char(begin, '\n') + 1;
    str_glue(out, cin, end - cin);
    
    return true;
}

String parse_shader_includes(Arena &a, String s) {
    constexpr String INCLUDE = S("#include");

    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    String t = s;
    String inc = str_slice(t, INCLUDE);
    
    String_Builder sb;
    
    // @Todo: handle recursive includes.
    while (is_valid(inc)) {
        String inc_left = str_slice(t, INCLUDE, S_LEFT_SLICE_BIT | S_INDEX_MINUS_ONE_BIT);
        str_build(a, sb, inc_left);

        String inc_path = str_slice(inc,      '"', S_INDEX_PLUS_ONE_BIT);
        inc_path        = str_slice(inc_path, '"', S_LEFT_SLICE_BIT | S_INDEX_MINUS_ONE_BIT);
        
        String_Builder sb_path;
        str_build(scratch.arena, sb_path, DIR_SHADERS);
        str_build(scratch.arena, sb_path, inc_path);

        String path = str_build_finish(scratch.arena, sb_path);
        String inc_contents = os_read_text_file(scratch.arena, path);
        str_build(a, sb, inc_contents);

        // @Robustness: this may lead to undesired behavior if several
        // include statements will be on same line.
        t   = str_slice(inc, '\n', S_INDEX_PLUS_ONE_BIT);
        inc = str_slice(t, INCLUDE);
    }

    str_build(a, sb, t); // rest of shader code

    String r = str_build_finish(a, sb);
    if (r.length > R_Shader::MAX_FILE_SIZE) {
        error("Final shader source size %llu with parsed includes exceeds max shade file size %llu", r.length, R_Shader::MAX_FILE_SIZE);
    }
    
    return r;
}

bool parse_shader_regions(const char *source, char *out_vertex, char *out_fragment) {
    if (!parse_shader_region(source, out_vertex, REGION_VERTEX)) {
        error("Failed to parse vertex shader region");
        return false;
    }

    if (!parse_shader_region(source, out_fragment, REGION_FRAGMENT)) {
        error("Failed to parse fragment shader region");
        return false;
    }

    return true;
}
