#include "pch.h"
#include "render/shader.h"

#include "log.h"

#include <string.h>

static const char *vertex_region_begin_name   = "@vertex_begin";
static const char *vertex_region_end_name     = "@vertex_end";
static const char *fragment_region_begin_name = "@fragment_begin";
static const char *fragment_region_end_name   = "@fragment_end";

static bool parse_shader_region(const char *shader_src, char *region_src, const char *region_begin_name, const char *region_end_name) {
    const char *region_begin = strstr(shader_src, region_begin_name);
    const char *region_end   = strstr(shader_src, region_end_name);

    if (!region_begin || !region_end) {
        error("Failed to find shader region %s ... %s", region_begin_name, region_end_name);
        return false;
    }

    region_begin += strlen(region_begin_name);

    const u64 region_size = region_end - region_begin;
    if (region_size >= MAX_SHADER_SIZE) {
        error("Shader region %s ... %s size is too big %u", region_begin_name, region_end_name, region_size);
        return false;
    }
    
    memcpy(region_src, region_begin, region_size);
	region_src[region_size] = '\0';

    return true;
}

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src) {
    if (!parse_shader_region(shader_src, vertex_src, vertex_region_begin_name, vertex_region_end_name)) {
        error("Failed to parse vertex shader region");
        return false;
    }

    if (!parse_shader_region(shader_src, fragment_src, fragment_region_begin_name, fragment_region_end_name)) {
        error("Failed to parse fragment shader region");
        return false;
    }

    return true;
}
