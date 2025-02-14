#pragma once

inline constexpr s32 MAX_SHADER_SIZE = KB(8);

struct Shader
{
    u32 id;

    // @Todo: make shader parser, so this will collapse to 1 shader file.
    const char* path;
};

struct Shader_List
{
    Shader player;
    Shader text;
    Shader skybox;
};

// Only 1 shader can be changed at time, are we fine with it?
// @Cleanup: its better to make queue of hot reload shaders.
inline Shader* shader_to_hot_reload = null;
inline Shader_List shaders;

void compile_game_shaders(Shader_List* list);
Shader create_shader(const char* shader_path);
Shader* find_shader_by_file(Shader_List* list, const char* path);

// @Cleanup: current hot-reload implementation is not actually thread-safe!
void hot_reload_shader(Shader* shader);
void on_shader_changed_externally(const char* relative_path);
void check_shader_to_hot_reload();
