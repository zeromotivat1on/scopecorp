#include "pch.h"
#include "shader.h"
#include "thread.h"
#include "file.h"
#include "gl.h"
#include <string.h>

const char* vertex_region_name = "[vertex]";
const char* fragment_region_name = "[fragment]";

static void parse_shader_source(const char* path, const char* shader_src, char* vertex_src, char* fragment_src)
{
    static u64 vertex_region_name_size = strlen(vertex_region_name);
    static u64 fragment_region_name_size = strlen(fragment_region_name);

    const char* vertex_region = strstr(shader_src, vertex_region_name);
    if (!vertex_region)
    {
        log("Failed to find vertex region in shader (%s)", path);
        return;
    }
    
    const char* fragment_region = strstr(shader_src, fragment_region_name);
    if (!fragment_region)
    {
        log("Failed to find fragment region in shader (%s)", path);
        return;
    }
    
    vertex_region += vertex_region_name_size;
    const s32 vertex_src_size = (s32)(fragment_region - vertex_region);

    fragment_region += fragment_region_name_size;
    const char* end_pos = shader_src + strlen(shader_src);
    const s32 fragment_src_size = (s32)(end_pos - fragment_region);
    
    memcpy(vertex_src, vertex_region, vertex_src_size);
    memcpy(fragment_src, fragment_region, fragment_src_size);
}

void compile_game_shaders(Shader_List* list)
{
    list->player = create_shader(DIR_SHADERS "player.glsl");
    list->text   = create_shader(DIR_SHADERS "text.glsl");
}

Shader create_shader(const char* shader_path)
{
    Shader shader;
    shader.path = shader_path;
    
    u64 shader_size = 0;
    u8* shader_src = alloc_buffer_temp(MAX_SHADER_SIZE);
    if (!read_entire_file(shader_path, shader_src, MAX_SHADER_SIZE, &shader_size))
    {
        free_buffer_temp(MAX_SHADER_SIZE);
        return {0};
    }
    shader_src[shader_size] = '\0';

    char* vertex_src = (char*)alloc_buffer_temp(MAX_SHADER_SIZE);
    char* fragment_src = (char*)alloc_buffer_temp(MAX_SHADER_SIZE);
    parse_shader_source(shader_path, (char*)shader_src, vertex_src, fragment_src);

    const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
    const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
    shader.id = gl_link_program(vertex_shader, fragment_shader);

    return shader;
}

Shader* find_shader_by_file(Shader_List* list, const char* path)
{
    const s32 item_size = sizeof(Shader);
    const s32 list_size = sizeof(Shader_List);
    
    u8* data = (u8*)list;
    s32 pos = 0;

    while (pos < list_size)
    {
        Shader* shader = (Shader*)(data + pos);
        if (strcmp(shader->path, path) == 0) return shader;
        pos += item_size;
    }

    return null;
}

void hot_reload_shader(Shader* shader)
{
    log("Hot reloading shader (%u) (%s)", shader->id, shader->path);

    assert(shader->id);
    glDeleteProgram(shader->id); // @Cleanup: render api layer here

    *shader = create_shader(shader->path);
}

void on_shader_changed_externally(const char* relative_path)
{
    // File change notification sends path relative to registered hot reload directory,
    // so we need to make full path out of it.
    char full_path[128];
    strcpy_s(full_path, sizeof(full_path), DIR_SHADERS);
    strcat_s(full_path, sizeof(full_path), relative_path);
    
    Shader* shader = find_shader_by_file(&shaders, full_path);
    if (shader) atomic_swap((void**)&shader_to_hot_reload, shader);
}

void check_shader_to_hot_reload()
{
    Shader* shader = (Shader*)atomic_swap((void**)&shader_to_hot_reload, null);
    if (shader) hot_reload_shader(shader);
}
