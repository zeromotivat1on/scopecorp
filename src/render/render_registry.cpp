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
