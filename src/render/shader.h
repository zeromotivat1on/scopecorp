#pragma once

#include "asset.h"

inline constexpr s32 MAX_SHADER_SIZE = KB(64);

typedef u64 sid;

struct Shader : Asset {
    Shader() { asset_type = ASSET_SHADER; }
    
	rid rid = RID_NONE;
};

struct Shader_Include : Asset {
    Shader_Include() { asset_type = ASSET_SHADER_INCLUDE; }
    
	void *data = null;
};

void init_shader_asset(Shader *shader, void *data);
bool parse_shader(const char *source, char *out_vertex, char *out_fragment);

const char *get_desired_shader_file_extension();
