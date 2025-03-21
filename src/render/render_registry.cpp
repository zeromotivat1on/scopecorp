#include "pch.h"
#include "render/render_registry.h"
#include "render/text.h"

void init_render_registry(Render_Registry *registry) {
	registry->frame_buffers  = Sparse_Array<Frame_Buffer>(MAX_FRAME_BUFFERS);
	registry->vertex_buffers = Sparse_Array<Vertex_Buffer>(MAX_VERTEX_BUFFERS);
	registry->index_buffers  = Sparse_Array<Index_Buffer>(MAX_INDEX_BUFFERS);
	registry->shaders        = Sparse_Array<Shader>(MAX_SHADERS);
	registry->textures       = Sparse_Array<Texture>(MAX_TEXTURES);
	registry->materials      = Sparse_Array<Material>(MAX_MATERIALS);
}

void compile_game_shaders(Shader_Index_List *list) {
    list->entity = create_shader(DIR_SHADERS "entity.glsl");
	list->text = create_shader(DIR_SHADERS "text.glsl");
	list->skybox = create_shader(DIR_SHADERS "skybox.glsl");
    list->frame_buffer = create_shader(DIR_SHADERS "frame_buffer.glsl");
    list->debug_geometry = create_shader(DIR_SHADERS "debug_geometry.glsl");
}

void load_game_textures(Texture_Index_List *list) {
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

void create_game_materials(Material_Index_List *list) {
    const Uniform text_uniforms[] = {
        Uniform("u_transforms", UNIFORM_F32_4X4, TEXT_DRAW_BATCH_SIZE),
        Uniform("u_projection", UNIFORM_F32_4X4, 1),
        Uniform("u_charmap",    UNIFORM_U32,     TEXT_DRAW_BATCH_SIZE),
        Uniform("u_color",      UNIFORM_F32_3,   1),
	};
        
    const Uniform entity_uniforms[] = {
        Uniform("u_transform", UNIFORM_F32_4X4, 1),
        Uniform("u_uv_scale",  UNIFORM_F32_2, 1),
    };

    const Uniform skybox_uniforms[] = {
		Uniform("u_scale",  UNIFORM_F32_2, 1),
		Uniform("u_offset", UNIFORM_F32_3, 1),
	};

    const s32 text_uniform_count   = c_array_count(text_uniforms);
    const s32 entity_uniform_count = c_array_count(entity_uniforms);
    const s32 skybox_uniform_count = c_array_count(skybox_uniforms);

    const s32 text_shader   = shader_index_list.text;
    const s32 entity_shader = shader_index_list.entity;
    const s32 skybox_shader = shader_index_list.skybox;

    const s32 skybox_texture = texture_index_list.skybox;
    const s32 ground_texture = texture_index_list.grass;
    const s32 cube_texture   = texture_index_list.stone;
    
    list->text = render_registry.materials.add(Material(text_shader, INVALID_INDEX));
	add_material_uniforms(list->text, text_uniforms, text_uniform_count);
    
	list->skybox = render_registry.materials.add(Material(skybox_shader, skybox_texture));
	add_material_uniforms(list->skybox, skybox_uniforms, skybox_uniform_count);

    list->player = render_registry.materials.add(Material(entity_shader, INVALID_INDEX));
	add_material_uniforms(list->player, entity_uniforms, entity_uniform_count);

	list->ground = render_registry.materials.add(Material(entity_shader, ground_texture));
	add_material_uniforms(list->ground, entity_uniforms, entity_uniform_count);

	list->cube = render_registry.materials.add(Material(entity_shader, cube_texture));
    add_material_uniforms(list->cube, entity_uniforms, entity_uniform_count);
}
