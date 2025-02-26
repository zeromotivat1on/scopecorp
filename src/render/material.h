#pragma once

inline constexpr s32 MAX_MATERIAL_UNIFORMS = 8;

struct Uniform;
struct Material
{
    Material() = default;
    Material(s32 shader_idx, s32 texture_idx)
        : shader_idx(shader_idx), texture_idx(texture_idx) {}
    
    s32 shader_idx = INVALID_INDEX;
    s32 texture_idx = INVALID_INDEX;
    s32 uniform_count = 0;
    Uniform uniforms[MAX_MATERIAL_UNIFORMS];
};

struct Material_Index_List {
    s32 player;
    s32 text;
    s32 skybox;
    s32 ground;
    s32 cube;
};

inline Material_Index_List material_index_list;

void create_game_materials(Material_Index_List* list);
void add_material_uniforms(s32 material_idx, const Uniform* uniforms, s32 count);
Uniform* find_material_uniform(s32 material_idx, const char* name);

// Given data is not copied to uniform, it just stores a reference to it.
// Caller should know value type in uniform to avoid possible read access violations.
// For now its ok to link local variables as long as they live till draw queue flush,
// where all uniforms are synced with gpu.
// @Cleanup: create large buffer for uniform values and store offsets in them?
void set_material_uniform_value(s32 material_idx, const char* name, const void* data);
void mark_material_uniform_dirty(s32 material_idx, const char* name);
