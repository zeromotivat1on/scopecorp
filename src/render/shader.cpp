#include "pch.h"
#include "shader.h"
#include "os/thread.h"
#include "os/file.h"
#include "gl.h"
#include "profile.h"
#include <stdio.h>
#include <string.h>

const char* vertex_region_name = "[vertex]";
const char* fragment_region_name = "[fragment]";

void compile_game_shaders(Shader_List* list)
{
    list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
    list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
    
    list->player = create_shader(DIR_SHADERS "player.glsl");
    list->text   = create_shader(DIR_SHADERS "text.glsl");
    list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
}

static void parse_shader_source(const char* path, const char* shader_src, char* vertex_src, char* fragment_src)
{
    static u64 vertex_region_name_size = strlen(vertex_region_name);
    static u64 fragment_region_name_size = strlen(fragment_region_name);

    const char* vertex_region = strstr(shader_src, vertex_region_name);
    if (!vertex_region)
    {
        error("Failed to find vertex region in shader %s", path);
        return;
    }
    
    const char* fragment_region = strstr(shader_src, fragment_region_name);
    if (!fragment_region)
    {
        error("Failed to find fragment region in shader %s", path);
        return;
    }
    
    vertex_region += vertex_region_name_size;
    const s32 vertex_src_size = (s32)(fragment_region - vertex_region);

    fragment_region += fragment_region_name_size;
    const char* end_pos = shader_src + strlen(shader_src);
    const s32 fragment_src_size = (s32)(end_pos - fragment_region);
    
    memcpy(vertex_src, vertex_region, vertex_src_size);
    vertex_src[vertex_src_size] = '\0';
    
    memcpy(fragment_src, fragment_region, fragment_src_size);
    fragment_src[fragment_src_size] = '\0';
}

Shader create_shader(const char* path)
{
    char timer_string[256];
    sprintf_s(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
    SCOPE_TIMER(timer_string);
    
    Shader shader;
    shader.path = path;

    u64 shader_size = 0;
    u8* shader_src = alloc_buffer_temp(MAX_SHADER_SIZE);
    if (!read_file(path, shader_src, MAX_SHADER_SIZE, &shader_size))
    {
        free_buffer_temp(MAX_SHADER_SIZE);
        return {0};
    }
    
    shader_src[shader_size] = '\0';

    char* vertex_src = (char*)alloc_buffer_temp(MAX_SHADER_SIZE);
    char* fragment_src = (char*)alloc_buffer_temp(MAX_SHADER_SIZE);
    parse_shader_source(path, (char*)shader_src, vertex_src, fragment_src);
    free_buffer_temp(MAX_SHADER_SIZE); // free shader source
    
    const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
    const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
    shader.id = gl_link_program(vertex_shader, fragment_shader);

    free_temp(MAX_SHADER_SIZE); // free vertex source
    free_temp(MAX_SHADER_SIZE); // free fragment source
    
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

void init_shader_hot_reload(Shader_Hot_Reload_Queue* queue)
{
    *queue = {0};
}

bool hot_reload_shader(Shader* shader)
{
    assert(shader->id);
    u32 old_id = shader->id;

    Shader new_shader = create_shader(shader->path);
    if (new_shader.id == 0)
    {
        error("Failed shader hot reload for %s, see errors above", shader->path);
        return false;
    }

    log("Hot reloaded shader %s", shader->path);

    glDeleteProgram(old_id);
    *shader = new_shader;
    return true;
}

void on_shader_changed_externally(const char* path)
{
    Shader* shader = find_shader_by_file(&shaders, path);
    if (!shader) return;

    bool shader_already_in_queue = false;
    for (s32 i = 0; i < shader_hot_reload_queue.count; ++i)
    {
        if (shader == shader_hot_reload_queue.shaders[i])
            shader_already_in_queue = true;
    }
    
    if (!shader_already_in_queue)
    {
        assert(shader_hot_reload_queue.count < MAX_SHADER_HOT_RELOAD_QUEUE_SIZE);
        shader_hot_reload_queue.shaders[shader_hot_reload_queue.count] = shader;
        shader_hot_reload_queue.count++;        
    }
}

void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue* queue)
{
    if (queue->count == 0) return;
    
    for (s32 i = 0; i < queue->count; ++i)
    {
        Shader* shader = queue->shaders[i];
        // @Cleanup: not correct in case of success of not last shader in queue.
        if (hot_reload_shader(shader)) queue->count--;
    }    
}
