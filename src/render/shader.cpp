#include "pch.h"
#include "shader.h"
#include "render_registry.h"
#include "os/thread.h"
#include "os/file.h"
#include "gl.h"
#include "log.h"
#include "profile.h"
#include <stdio.h>
#include <string.h>

void compile_game_shaders(Shader_Index_List* list)
{
    Uniform u_mvp = Uniform("u_mvp", UNIFORM_F32_MAT4, 1);

    list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
    add_shader_uniforms(list->pos_col, &u_mvp, 1);
    
    list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
    add_shader_uniforms(list->pos_tex, &u_mvp, 1);
        
    list->player = create_shader(DIR_SHADERS "player.glsl");
    add_shader_uniforms(list->player, &u_mvp, 1);
    
    list->text = create_shader(DIR_SHADERS "text.glsl");
    Uniform text_uniforms[] = {
        Uniform("u_charmap",    UNIFORM_U32,      128),
        Uniform("u_transforms", UNIFORM_F32_MAT4, 128),
        Uniform("u_projection", UNIFORM_F32_MAT4, 1),
        Uniform("u_text_color", UNIFORM_F32_VEC3, 1),
    };
    add_shader_uniforms(list->text, text_uniforms, c_array_count(text_uniforms));
    
    list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
    Uniform u_scale = Uniform("u_scale", UNIFORM_F32_VEC2, 1);
    Uniform u_offset = Uniform("u_offset", UNIFORM_F32_VEC3, 1);
    add_shader_uniforms(list->skybox, &u_scale, 1);
    add_shader_uniforms(list->skybox, &u_offset, 1);
}
