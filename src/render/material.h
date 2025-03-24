#pragma once

inline constexpr s32 MAX_MATERIAL_UNIFORMS = 8;

struct Material {
	s32 shader_index  = INVALID_INDEX;
	s32 texture_index = INVALID_INDEX;

    s32 uniform_count = 0;
	s32 uniform_indices      [MAX_MATERIAL_UNIFORMS];
    s32 uniform_value_offsets[MAX_MATERIAL_UNIFORMS];
};

struct Material_Index_List {
	s32 player;
	s32 text;
	s32 skybox;
	s32 ground;
	s32 cube;
	s32 outline;
};

inline Material_Index_List material_index_list;

void create_game_materials(Material_Index_List *list);
//void add_material_uniforms(s32 material_index, const Uniform *uniforms, s32 count);
s32  create_material(s32 shader_index, s32 texture_index);
s32  find_material_uniform(s32 material_index, const char *name);
void set_material_uniforms(s32 material_index, const s32 *uniform_indices, s32 count);
void set_material_uniform_value(s32 material_index, const char *name, const void *data);
//void mark_material_uniform_dirty(s32 material_index, const char *name);

// Given data is not copied to uniform, it just stores a reference to it.
// Caller should know value type in uniform to avoid possible read access violations.
// For now its ok to link local variables as long as they live till draw queue flush,
// where all uniforms are synced with gpu, or you can allocate them in frame memory
// storage as its cleared after draw commands are flushed.
// @Cleanup: create large buffer for uniform values and store offsets in them?
//void set_material_uniform_value(s32 material_index, const char *name, const void *data);
//void mark_material_uniform_dirty(s32 material_index, const char *name);
