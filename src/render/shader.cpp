#include "pch.h"
#include "render/shader.h"

#include "log.h"

#include <string.h>

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src) {
	static const u64 vertex_region_name_size = strlen(vertex_region_name);
	static const u64 fragment_region_name_size = strlen(fragment_region_name);

	const char *vertex_region = strstr(shader_src, vertex_region_name);
	if (!vertex_region) {
		error("Failed to find shader vertex region");
		return false;
	}

	const char *fragment_region = strstr(shader_src, fragment_region_name);
	if (!fragment_region) {
		error("Failed to find shader fragment region");
		return false;
	}

	vertex_region += vertex_region_name_size;
	const s32 vertex_src_size = (s32)(fragment_region - vertex_region);

	fragment_region += fragment_region_name_size;
	const char *end_pos = shader_src + strlen(shader_src);
	const s32 fragment_src_size = (s32)(end_pos - fragment_region);

	memcpy(vertex_src, vertex_region, vertex_src_size);
	vertex_src[vertex_src_size] = '\0';

	memcpy(fragment_src, fragment_region, fragment_src_size);
	fragment_src[fragment_src_size] = '\0';

    return true;
}
