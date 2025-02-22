#include "pch.h"
#include "shader.h"
#include "render_registry.h"
#include "os/thread.h"
#include "os/file.h"
#include "gl.h"
#include "profile.h"
#include <stdio.h>
#include <string.h>

const char* vertex_region_name = "[vertex]";
const char* fragment_region_name = "[fragment]";

void compile_game_shaders(Shader_Index_List* list)
{
    Uniform u_mvp = create_uniform("u_mvp", UNIFORM_F32_MAT4, 1);

    list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
    add_shader_uniforms(list->pos_col, &u_mvp, 1);
    
    list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
    add_shader_uniforms(list->pos_tex, &u_mvp, 1);
        
    list->player = create_shader(DIR_SHADERS "player.glsl");
    add_shader_uniforms(list->player, &u_mvp, 1);
    
    list->text = create_shader(DIR_SHADERS "text.glsl");
    Uniform u_transforms = create_uniform("u_transforms", UNIFORM_F32_MAT4, 128);
    Uniform u_projection = create_uniform("u_projection", UNIFORM_F32_MAT4, 1);  
    add_shader_uniforms(list->text, &u_transforms, 1);
    add_shader_uniforms(list->text, &u_projection, 1);
    
    list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
    Uniform u_scale = create_uniform("u_scale", UNIFORM_F32_VEC2, 1);
    Uniform u_offset = create_uniform("u_offset", UNIFORM_F32_VEC3, 1);
    add_shader_uniforms(list->skybox, &u_scale, 1);
    add_shader_uniforms(list->skybox, &u_offset, 1);
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

s32 create_shader(const char* path)
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
        return INVALID_INDEX;
    }
    
    shader_src[shader_size] = '\0';

    char* vertex_src = (char*)alloc_temp(MAX_SHADER_SIZE);
    char* fragment_src = (char*)alloc_temp(MAX_SHADER_SIZE);
    parse_shader_source(path, (char*)shader_src, vertex_src, fragment_src);
    free_buffer_temp(MAX_SHADER_SIZE);
    
    const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
    const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
    shader.id = gl_link_program(vertex_shader, fragment_shader);

    free_temp(MAX_SHADER_SIZE);
    free_temp(MAX_SHADER_SIZE);
    
    return add_shader(&render_registry, &shader);
}

s32 recreate_shader(s32 shader_idx)
{
    if (shader_idx >= MAX_SHADERS) return INVALID_INDEX;

    Shader* shader = render_registry.shaders + shader_idx;

    u64 shader_size = 0;
    u8* shader_src = alloc_buffer_temp(MAX_SHADER_SIZE);
    if (!read_file(shader->path, shader_src, MAX_SHADER_SIZE, &shader_size))
    {
        free_buffer_temp(MAX_SHADER_SIZE);
        return INVALID_INDEX;
    }
    
    shader_src[shader_size] = '\0';

    char* vertex_src = (char*)alloc_temp(MAX_SHADER_SIZE);
    char* fragment_src = (char*)alloc_temp(MAX_SHADER_SIZE);
    parse_shader_source(shader->path, (char*)shader_src, vertex_src, fragment_src);
    free_buffer_temp(MAX_SHADER_SIZE);

    // We are free to delete old shader program at this stage.
    glDeleteProgram(shader->id);

    const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
    const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
    shader->id = gl_link_program(vertex_shader, fragment_shader);

    free_temp(MAX_SHADER_SIZE);
    free_temp(MAX_SHADER_SIZE);
    
    return shader_idx;
}

s32 find_shader_by_file(Shader_Index_List* list, const char* path)
{
    const s32 item_size = sizeof(s32);
    const s32 list_size = sizeof(Shader_Index_List);
    
    u8* data = (u8*)list;
    s32 pos = 0;

    while (pos < list_size)
    {
        s32 shader_idx = *(s32*)(data + pos);
        assert(shader_idx < MAX_SHADERS);

        Shader* shader = render_registry.shaders + shader_idx;
        if (strcmp(shader->path, path) == 0) return shader_idx;

        pos += item_size;
    }

    return INVALID_INDEX;
}

void add_shader_uniforms(s32 shader_idx, Uniform* uniforms, s32 count)
{
    assert(shader_idx < MAX_SHADERS);
    Shader* shader = render_registry.shaders + shader_idx;
    assert(shader->uniform_count + count <= MAX_SHADER_UNIFORMS);
    
    for (s32 i = 0; i < count; ++i)
    {
        auto* other_uniform  = uniforms + i;
        auto* shader_uniform = shader->uniforms + shader->uniform_count + i;
        memcpy(shader_uniform, other_uniform, sizeof(Uniform));
        shader_uniform->location = glGetUniformLocation(shader->id, shader_uniform->name);   
    }

    shader->uniform_count += count;
}

void set_shader_uniform_value(s32 shader_idx, const char* name, const void* data)
{
    assert(shader_idx < MAX_SHADERS);
    Shader* shader = render_registry.shaders + shader_idx;
    
    Uniform* uniform = null;
    for (s32 i = 0; i < shader->uniform_count; ++i)
    {
        if (strcmp(shader->uniforms[i].name, name) == 0)
           uniform = shader->uniforms + i;
    }

    if (!uniform)
    {
        error("Failed to set shader uniform value as its name not found %s", name);
        return;
    }

    switch(uniform->type)
    {
    case UNIFORM_F32_VEC2: uniform->_vec2 = vec2((f32*)data); break;
    case UNIFORM_F32_VEC3: uniform->_vec3 = vec3((f32*)data); break;
    case UNIFORM_F32_MAT4: uniform->_mat4 = mat4((f32*)data); break;
    default:
        error("Failed to update shader uniform value of unknown type %d", uniform->type);
        break;
    }
}

void init_shader_hot_reload(Shader_Hot_Reload_Queue* queue)
{
    *queue = {0};
}

// @Cleanup: do not need this as recreate_shader exists now?
bool hot_reload_shader(s32 shader_idx)
{
    assert(shader_idx < MAX_SHADERS);
    Shader* shader = render_registry.shaders + shader_idx;

    shader_idx = recreate_shader(shader_idx);
    if (shader_idx == INVALID_INDEX)
    {
        error("Failed to hot reload shader %s, see errors above", shader->path);
        return false;
    }

    log("Hot reloaded shader %s", shader->path);
    return true;
}

void on_shader_changed_externally(const char* path)
{
    const s32 shader_idx = find_shader_by_file(&shader_index_list, path);
    if (shader_idx == INVALID_INDEX) return;

    bool already_in_queue = false;
    for (s32 i = 0; i < shader_hot_reload_queue.count; ++i)
    {
        if (shader_idx == shader_hot_reload_queue.indices[i])
            already_in_queue = true;
    }
    
    if (!already_in_queue)
    {
        assert(shader_hot_reload_queue.count < MAX_SHADER_HOT_RELOAD_QUEUE_SIZE);
        shader_hot_reload_queue.indices[shader_hot_reload_queue.count] = shader_idx;
        shader_hot_reload_queue.count++;
    }
}

void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue* queue)
{
    if (queue->count == 0) return;
    
    for (s32 i = 0; i < queue->count; ++i)
    {
        const s32 idx = queue->indices[i];
        // @Cleanup: not correct in case of success of not last shader in queue.
        if (hot_reload_shader(idx)) queue->count--;
    }    
}
