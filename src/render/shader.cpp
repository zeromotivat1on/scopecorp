#include "pch.h"
#include "shader.h"

void compile_game_shaders(Shader_Index_List* list)
{
    list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
    list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
    list->player = create_shader(DIR_SHADERS "player.glsl");
    list->text = create_shader(DIR_SHADERS "text.glsl");
    list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
}
