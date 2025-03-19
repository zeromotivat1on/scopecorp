#include "pch.h"
#include "render/render_registry.h"
#include "render/text.h"

void init_render_registry(Render_Registry *registry)
{
	registry->frame_buffers  = Sparse_Array<Frame_Buffer>(MAX_FRAME_BUFFERS);
	registry->vertex_buffers = Sparse_Array<Vertex_Buffer>(MAX_VERTEX_BUFFERS);
	registry->index_buffers  = Sparse_Array<Index_Buffer>(MAX_INDEX_BUFFERS);
	registry->shaders        = Sparse_Array<Shader>(MAX_SHADERS);
	registry->textures       = Sparse_Array<Texture>(MAX_TEXTURES);
	registry->materials      = Sparse_Array<Material>(MAX_MATERIALS);
}

void compile_game_shaders(Shader_Index_List *list)
{
	list->pos_col = create_shader(DIR_SHADERS "pos_col.glsl");
	list->pos_tex = create_shader(DIR_SHADERS "pos_tex.glsl");
	list->pos_tex_scale = create_shader(DIR_SHADERS "pos_tex_scale.glsl");
    list->debug_geometry = create_shader(DIR_SHADERS "debug_geometry.glsl");
	list->player = create_shader(DIR_SHADERS "player.glsl");
	list->text = create_shader(DIR_SHADERS "text.glsl");
	list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
}

void load_game_textures(Texture_Index_List *list)
{
	list->skybox = create_texture(DIR_TEXTURES "skybox.png");
	list->stone  = create_texture(DIR_TEXTURES "stone.png");
	list->grass  = create_texture(DIR_TEXTURES "grass.png");

	list->player_idle[DIRECTION_BACK] = create_texture(DIR_TEXTURES "player_idle_back.png");
	list->player_idle[DIRECTION_RIGHT] = create_texture(DIR_TEXTURES "player_idle_right.png");
	list->player_idle[DIRECTION_LEFT] = create_texture(DIR_TEXTURES "player_idle_left.png");
	list->player_idle[DIRECTION_FORWARD] = create_texture(DIR_TEXTURES "player_idle_forward.png");

	list->player_move[DIRECTION_BACK][0] = create_texture(DIR_TEXTURES "player_move_back_1.png");
	list->player_move[DIRECTION_BACK][1] = create_texture(DIR_TEXTURES "player_move_back_2.png");
	list->player_move[DIRECTION_BACK][2] = create_texture(DIR_TEXTURES "player_move_back_3.png");
	list->player_move[DIRECTION_BACK][3] = create_texture(DIR_TEXTURES "player_move_back_4.png");

	list->player_move[DIRECTION_RIGHT][0] = create_texture(DIR_TEXTURES "player_move_right_1.png");
	list->player_move[DIRECTION_RIGHT][1] = create_texture(DIR_TEXTURES "player_move_right_2.png");
	list->player_move[DIRECTION_RIGHT][2] = create_texture(DIR_TEXTURES "player_move_right_3.png");
	list->player_move[DIRECTION_RIGHT][3] = create_texture(DIR_TEXTURES "player_move_right_4.png");

	list->player_move[DIRECTION_LEFT][0] = create_texture(DIR_TEXTURES "player_move_left_1.png");
	list->player_move[DIRECTION_LEFT][1] = create_texture(DIR_TEXTURES "player_move_left_2.png");
	list->player_move[DIRECTION_LEFT][2] = create_texture(DIR_TEXTURES "player_move_left_3.png");
	list->player_move[DIRECTION_LEFT][3] = create_texture(DIR_TEXTURES "player_move_left_4.png");

	list->player_move[DIRECTION_FORWARD][0] = create_texture(DIR_TEXTURES "player_move_forward_1.png");
	list->player_move[DIRECTION_FORWARD][1] = create_texture(DIR_TEXTURES "player_move_forward_2.png");
	list->player_move[DIRECTION_FORWARD][2] = create_texture(DIR_TEXTURES "player_move_forward_3.png");
	list->player_move[DIRECTION_FORWARD][3] = create_texture(DIR_TEXTURES "player_move_forward_4.png");
}

void create_game_materials(Material_Index_List *list)
{
	const Uniform u_mvp = Uniform("u_mvp", UNIFORM_F32_MAT4, 1);

	list->player = render_registry.materials.add(Material(shader_index_list.player, INVALID_INDEX));
	add_material_uniforms(list->player, &u_mvp);

	list->text = render_registry.materials.add(Material(shader_index_list.text, INVALID_INDEX));
	const Uniform text_uniforms[] = {
		Uniform("u_charmap",    UNIFORM_U32,      TEXT_DRAW_BATCH_SIZE),
		Uniform("u_transforms", UNIFORM_F32_MAT4, TEXT_DRAW_BATCH_SIZE),
		Uniform("u_projection", UNIFORM_F32_MAT4),
		Uniform("u_text_color", UNIFORM_F32_VEC3),
	};
	add_material_uniforms(list->text, text_uniforms, c_array_count(text_uniforms));

	list->skybox = render_registry.materials.add(Material(shader_index_list.skybox, texture_index_list.skybox));
	const Uniform skybox_uniforms[] = {
		Uniform("u_scale", UNIFORM_F32_VEC2),
		Uniform("u_offset", UNIFORM_F32_VEC3),
	};
	add_material_uniforms(list->skybox, skybox_uniforms, c_array_count(skybox_uniforms));

	list->ground = render_registry.materials.add(Material(shader_index_list.pos_tex_scale, texture_index_list.grass));
	const Uniform ground_uniforms[] = {
		u_mvp,
		Uniform("u_scale", UNIFORM_F32_VEC2),
	};
	add_material_uniforms(list->ground, ground_uniforms, c_array_count(ground_uniforms));

	list->cube = render_registry.materials.add(Material(shader_index_list.pos_tex, texture_index_list.stone));
	add_material_uniforms(list->cube, &u_mvp);
}
