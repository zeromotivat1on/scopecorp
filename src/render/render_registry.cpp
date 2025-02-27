#include "pch.h"
#include "render/render_registry.h"

void init_render_registry(Render_Registry* registry)
{
    registry->vertex_buffers = Array<Vertex_Buffer>(MAX_VERTEX_BUFFERS);
    registry->index_buffers  = Array<Index_Buffer>(MAX_INDEX_BUFFERS);
    registry->shaders        = Array<Shader>(MAX_SHADERS);
    registry->textures       = Array<Texture>(MAX_TEXTURES);
    registry->materials      = Array<Material>(MAX_MATERIALS);
}

void compile_game_shaders(Shader_Index_List* list)
{
    list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
    list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
    list->player = create_shader(DIR_SHADERS "player.glsl");
    list->text = create_shader(DIR_SHADERS "text.glsl");
    list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
}

void load_game_textures(Texture_Index_List* list)
{
    list->skybox = create_texture(DIR_TEXTURES "skybox.png");
    list->stone  = create_texture(DIR_TEXTURES "stone.png");
    
    list->player_idle[BACK] = create_texture(DIR_TEXTURES "player_idle_back.png");
    list->player_idle[RIGHT] = create_texture(DIR_TEXTURES "player_idle_right.png");
    list->player_idle[LEFT] = create_texture(DIR_TEXTURES "player_idle_left.png");
    list->player_idle[FORWARD] = create_texture(DIR_TEXTURES "player_idle_forward.png");

    list->player_move[BACK][0] = create_texture(DIR_TEXTURES "player_move_back_1.png");
    list->player_move[BACK][1] = create_texture(DIR_TEXTURES "player_move_back_2.png");
    list->player_move[BACK][2] = create_texture(DIR_TEXTURES "player_move_back_3.png");
    list->player_move[BACK][3] = create_texture(DIR_TEXTURES "player_move_back_4.png");

    list->player_move[RIGHT][0] = create_texture(DIR_TEXTURES "player_move_right_1.png");
    list->player_move[RIGHT][1] = create_texture(DIR_TEXTURES "player_move_right_2.png");
    list->player_move[RIGHT][2] = create_texture(DIR_TEXTURES "player_move_right_3.png");
    list->player_move[RIGHT][3] = create_texture(DIR_TEXTURES "player_move_right_4.png");

    list->player_move[LEFT][0] = create_texture(DIR_TEXTURES "player_move_left_1.png");
    list->player_move[LEFT][1] = create_texture(DIR_TEXTURES "player_move_left_2.png");
    list->player_move[LEFT][2] = create_texture(DIR_TEXTURES "player_move_left_3.png");
    list->player_move[LEFT][3] = create_texture(DIR_TEXTURES "player_move_left_4.png");

    list->player_move[FORWARD][0] = create_texture(DIR_TEXTURES "player_move_forward_1.png");
    list->player_move[FORWARD][1] = create_texture(DIR_TEXTURES "player_move_forward_2.png");
    list->player_move[FORWARD][2] = create_texture(DIR_TEXTURES "player_move_forward_3.png");
    list->player_move[FORWARD][3] = create_texture(DIR_TEXTURES "player_move_forward_4.png");
}

void create_game_materials(Material_Index_List* list)
{
    const Uniform u_mvp = Uniform("u_mvp", UNIFORM_F32_MAT4, 1);

    list->player = render_registry.materials.add(Material(shader_index_list.player, INVALID_INDEX));
    add_material_uniforms(list->player, &u_mvp, 1);
    
    list->text = render_registry.materials.add(Material(shader_index_list.text, INVALID_INDEX));
    const Uniform text_uniforms[] = {
        Uniform("u_charmap",    UNIFORM_U32,      128),
        Uniform("u_transforms", UNIFORM_F32_MAT4, 128),
        Uniform("u_projection", UNIFORM_F32_MAT4, 1),
        Uniform("u_text_color", UNIFORM_F32_VEC3, 1),
    };
    add_material_uniforms(list->text, text_uniforms, c_array_count(text_uniforms));
    
    list->skybox = render_registry.materials.add(Material(shader_index_list.skybox, texture_index_list.skybox));
    const Uniform skybox_uniforms[] = {
        Uniform("u_scale", UNIFORM_F32_VEC2, 1),
        Uniform("u_offset", UNIFORM_F32_VEC3, 1),
    };
    add_material_uniforms(list->skybox, skybox_uniforms, c_array_count(skybox_uniforms));

    list->ground = render_registry.materials.add(Material(shader_index_list.pos_tex, texture_index_list.stone));
    add_material_uniforms(list->ground, &u_mvp, 1);

    list->cube = render_registry.materials.add(Material(shader_index_list.pos_tex, texture_index_list.stone));
    add_material_uniforms(list->cube, &u_mvp, 1);
}
