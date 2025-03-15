#pragma once

#include "uniform.h"

inline constexpr s32 MAX_SHADER_SIZE = KB(8);

struct Shader {
	u32 id;
	const char *path;
};

struct Shader_Index_List {
	s32 pos_col;
	s32 pos_tex;
	s32 pos_tex_scale;
	s32 debug_geometry;

	s32 player;
	s32 text;
	s32 skybox;
};

inline Shader_Index_List shader_index_list;

void compile_game_shaders(Shader_Index_List *list);

s32  create_shader(const char *path);
bool recreate_shader(s32 shader_index);
s32  find_shader_by_file(Shader_Index_List *list, const char *path);

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src);
