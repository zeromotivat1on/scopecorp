#include "pch.h"
#include "render/shader.h"

#include "log.h"
#include "str.h"
#include "asset.h"

#include "os/file.h"

enum Shader_Region_Type {
    REGION_VERTEX,
    REGION_FRAGMENT,
    REGION_COUNT
};

static const char *DECL_INCLUDE = "#include";

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

    cin = str_char(begin, '\n') + 1; // move to first character of actual shader source
    begin = cin;
    
    const char *include = str_sub(cin, DECL_INCLUDE);
    if (include) { // copy shader source before first include
        str_glue(out, cin, include - cin);
    }

    // @Todo: handle recursive includes.
    while (include && include < end) { // include files contents
        const char *na = str_char(include, '"') + 1;
        const char *nb = str_char(na,      '"') + 0;

        char path[MAX_PATH_SIZE];
        str_copy(path, DIR_SHADERS);
        str_glue(path, na, nb - na);

        char relative_path[MAX_PATH_SIZE];
        convert_to_relative_asset_path(relative_path, path);
        
        const auto &shader_include = asset_table.shader_includes[cache_sid(relative_path)];
        str_glue(out, (char *)shader_include.data, shader_include.data_size);

        // @Robustness: this may lead to undesired behavior if several
        // include statements will be on same line.
        cin = str_char(include, '\n') + 1;
        include = str_sub(cin, DECL_INCLUDE);
    }

    str_glue(out, cin, end - cin); // rest of shader code
    return true;
}

bool parse_shader(const char *source, char *out_vertex, char *out_fragment) {
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
