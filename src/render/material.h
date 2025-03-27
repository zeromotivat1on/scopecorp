#pragma once

inline constexpr s32 MAX_MATERIAL_UNIFORMS = 8;

struct Render_Command;

struct Material {
	s32 shader_index  = INVALID_INDEX;
	s32 texture_index = INVALID_INDEX;

    s32 uniform_count = 0;
	s32 uniform_indices      [MAX_MATERIAL_UNIFORMS];
    s32 uniform_value_offsets[MAX_MATERIAL_UNIFORMS];
};

struct Material_Index_List {
	s32 player;
	s32 skybox;
	s32 ground;
	s32 cube;
	s32 outline;
};

inline Material_Index_List material_index_list;

void create_game_materials(Material_Index_List *list);
s32  create_material(s32 shader_index, s32 texture_index);
s32  find_material_uniform(s32 material_index, const char *name);
void set_material_uniforms(s32 material_index, const s32 *uniform_indices, s32 count);
void set_material_uniform_value(s32 material_index, const char *name, const void *data);
void fill_render_command_with_material_data(s32 material_index, Render_Command* command);
