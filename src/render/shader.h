#pragma once

#include "thread.h"

inline constexpr s32 MAX_SHADER_SIZE = KB(8);
inline constexpr s32 MAX_SHADER_HOT_RELOAD_QUEUE_SIZE = 4;

struct Shader
{
    u32 id;

    // @Todo: make shader parser, so this will collapse to 1 shader file.
    const char* path;
};

struct Shader_List
{
    // General shaders.
    Shader pos_col;
    Shader pos_tex;

    // Specific shaders.
    Shader player;
    Shader text;
    Shader skybox;
};

struct Shader_Hot_Reload_Queue
{
    Shader* shaders[MAX_SHADER_HOT_RELOAD_QUEUE_SIZE];
    s32 count;
    Critical_Section cs;
};

inline Shader_List shaders;
inline Shader_Hot_Reload_Queue shader_hot_reload_queue;

void compile_game_shaders(Shader_List* list);
Shader create_shader(const char* path);
Shader* find_shader_by_file(Shader_List* list, const char* path);

// @Cleanup: current hot-reload implementation is not actually thread-safe!
void init_shader_hot_reload(Shader_Hot_Reload_Queue* queue);
bool hot_reload_shader(Shader* shader);
void on_shader_changed_externally(const char* path);
void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue* queue);
