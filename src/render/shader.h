#pragma once

#include "render/uniform.h"

inline constexpr s32 MAX_SHADER_SIZE = KB(8);

struct Shader {
	u32 id;
	const char *path;
};

struct Shader_Index_List {
	s32 entity;
	s32 text;
	s32 skybox;
	s32 frame_buffer;
    s32 geometry;
    s32 outline;
};

inline Shader_Index_List shader_index_list;

void compile_game_shaders(Shader_Index_List *list);

s32  create_shader(const char *path);
bool recreate_shader(s32 shader_index);
s32  find_shader_by_file(Shader_Index_List *list, const char *path);

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src);
