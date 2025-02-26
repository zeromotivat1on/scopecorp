#pragma once

#include "uniform.h"

inline constexpr s32 MAX_SHADER_SIZE = KB(8);
inline constexpr s32 MAX_SHADER_HOT_RELOAD_QUEUE_SIZE = 4;

inline const char* vertex_region_name   = "[vertex]";
inline const char* fragment_region_name = "[fragment]";

struct Shader {
    u32 id;
    const char* path;
};

struct Shader_Index_List {
    // General shaders.
    s32 pos_col;
    s32 pos_tex;

    // Specific shaders.
    s32 player;
    s32 text;
    s32 skybox;
};

struct Shader_Hot_Reload_Queue {
    s32 indices[MAX_SHADER_HOT_RELOAD_QUEUE_SIZE];
    s32 count = 0;
};

inline Shader_Index_List shader_index_list;
inline Shader_Hot_Reload_Queue shader_hot_reload_queue;

// @Cleanup: move such inits (textures, uniforms etc.) to render registry.
void compile_game_shaders(Shader_Index_List* list);

s32  create_shader(const char* path);
bool recreate_shader(Shader* shader);
s32  find_shader_by_file(Shader_Index_List* list, const char* path);

// @Cleanup: current hot-reload implementation is not actually thread-safe!
void init_shader_hot_reload(Shader_Hot_Reload_Queue* queue);
bool hot_reload_shader(s32 shader_idx);
void on_shader_changed_externally(const char* path);
void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue* queue);
