#include "pch.h"
#include "render/render_registry.h"
#include "render/text.h"

#include "log.h"

void init_render_registry(Render_Registry *registry) {
	registry->frame_buffers  = Sparse_Array<Frame_Buffer>(MAX_FRAME_BUFFERS);
	registry->vertex_buffers = Sparse_Array<Vertex_Buffer>(MAX_VERTEX_BUFFERS);
	registry->index_buffers  = Sparse_Array<Index_Buffer>(MAX_INDEX_BUFFERS);
	registry->shaders        = Sparse_Array<Shader>(MAX_SHADERS);
	registry->textures       = Sparse_Array<Texture>(MAX_TEXTURES);
	registry->materials      = Sparse_Array<Material>(MAX_MATERIALS);
	registry->uniforms       = Sparse_Array<Uniform>(MAX_UNIFORMS);

    registry->uniform_value_cache.data     = push(pers, MAX_UNIFORM_VALUE_CACHE_SIZE);
    registry->uniform_value_cache.size     = 0;
    registry->uniform_value_cache.capacity = MAX_UNIFORM_VALUE_CACHE_SIZE;
}

void compile_game_shaders(Shader_Index_List *list) {
    list->entity         = create_shader(DIR_SHADERS "entity.glsl");
	list->text           = create_shader(DIR_SHADERS "text.glsl");
	list->skybox         = create_shader(DIR_SHADERS "skybox.glsl");
    list->frame_buffer   = create_shader(DIR_SHADERS "frame_buffer.glsl");
    list->debug_geometry = create_shader(DIR_SHADERS "debug_geometry.glsl");
    list->outline        = create_shader(DIR_SHADERS "outline.glsl");
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
    const s32 text_uniforms[] = {
        create_uniform("u_transforms", UNIFORM_F32_4X4, TEXT_DRAW_BATCH_SIZE),
        create_uniform("u_projection", UNIFORM_F32_4X4, 1),
        create_uniform("u_charmap",    UNIFORM_U32,     TEXT_DRAW_BATCH_SIZE),
        create_uniform("u_color",      UNIFORM_F32_3,   1),
	};
        
    const s32 entity_uniforms[] = {
        create_uniform("u_transform", UNIFORM_F32_4X4, 1),
        create_uniform("u_uv_scale",  UNIFORM_F32_2,   1),
    };

    const s32 skybox_uniforms[] = {
		create_uniform("u_scale",  UNIFORM_F32_2, 1),
		create_uniform("u_offset", UNIFORM_F32_3, 1),
	};

    const s32 outline_uniforms[] = {
        create_uniform("u_transform", UNIFORM_F32_4X4, 1),
        create_uniform("u_color",     UNIFORM_F32_3,   1),
	};

    const s32 text_uniform_count    = c_array_count(text_uniforms);
    const s32 entity_uniform_count  = c_array_count(entity_uniforms);
    const s32 skybox_uniform_count  = c_array_count(skybox_uniforms);
    const s32 outline_uniform_count = c_array_count(outline_uniforms);

    const s32 text_shader    = shader_index_list.text;
    const s32 entity_shader  = shader_index_list.entity;
    const s32 skybox_shader  = shader_index_list.skybox;
    const s32 outline_shader = shader_index_list.outline;

    const s32 skybox_texture = texture_index_list.skybox;
    const s32 ground_texture = texture_index_list.grass;
    const s32 cube_texture   = texture_index_list.stone;
    
    list->text = create_material(text_shader, INVALID_INDEX);
	set_material_uniforms(list->text, text_uniforms, text_uniform_count);
    
	list->skybox = create_material(skybox_shader, skybox_texture);
	set_material_uniforms(list->skybox, skybox_uniforms, skybox_uniform_count);

    list->player = create_material(entity_shader, INVALID_INDEX);
	set_material_uniforms(list->player, entity_uniforms, entity_uniform_count);

	list->ground = create_material(entity_shader, ground_texture);
	set_material_uniforms(list->ground, entity_uniforms, entity_uniform_count);

	list->cube = create_material(entity_shader, cube_texture);
    set_material_uniforms(list->cube, entity_uniforms, entity_uniform_count);

    list->outline = create_material(outline_shader, INVALID_INDEX);
    set_material_uniforms(list->outline, outline_uniforms, outline_uniform_count);
}

s32 create_uniform(const char *name, Uniform_Type type, s32 element_count) {
    Uniform uniform;
    uniform.name  = name;
    uniform.type  = type;
    uniform.count = element_count;

    return render_registry.uniforms.add(uniform);
}

u32 cache_uniform_value_on_cpu(s32 uniform_index, const void *data) {
    const auto &uniform = render_registry.uniforms[uniform_index];
    auto &cache         = render_registry.uniform_value_cache;
    
    const u32 size = uniform.count * uniform_value_type_size(uniform.type);
    assert(cache.size + size <= cache.capacity);
        
    const u32 offset = cache.size;
    memcpy((u8 *)cache.data + cache.size, data, size);
    cache.size += size;

    return offset;
}

void update_uniform_value_on_cpu(s32 uniform_index, const void *data, u32 offset) {
    const auto &uniform = render_registry.uniforms[uniform_index];
    const auto &cache   = render_registry.uniform_value_cache;
    assert(offset < cache.size);
    
    const u32 size = uniform.count * uniform_value_type_size(uniform.type);
    memcpy((u8 *)cache.data + offset, data, size);
}

s32 create_material(s32 shader_index, s32 texture_index) {
    Material material;
    material.shader_index  = shader_index;
    material.texture_index = texture_index;
    
    for (s32 i = 0; i < MAX_MATERIAL_UNIFORMS; ++i) {
        material.uniform_indices[i]       = INVALID_INDEX;
        material.uniform_value_offsets[i] = INVALID_INDEX;
    }
    
    return render_registry.materials.add(material);
}

s32 find_material_uniform(s32 material_index, const char *name) {
	const auto &material = render_registry.materials[material_index];

	for (s32 i = 0; i < material.uniform_count; ++i) {
        const auto &uniform = render_registry.uniforms[material.uniform_indices[i]];
        if (strcmp(uniform.name, name) == 0) return i;
	}

	return INVALID_INDEX;
}

void set_material_uniforms(s32 material_index, const s32 *uniform_indices, s32 count) {
	auto &material     = render_registry.materials[material_index];
	const auto &shader = render_registry.shaders[material.shader_index];

    material.uniform_count = count;
    memcpy(material.uniform_indices, uniform_indices, count * sizeof(s32));
}

void set_material_uniform_value(s32 material_index, const char *name, const void *data) {
	const s32 material_uniform_index = find_material_uniform(material_index, name);
	if (material_uniform_index == INVALID_INDEX) {
		error("Failed to set material uniform %s value as its not found", name);
		return;
	}

    auto &material          = render_registry.materials[material_index];
    const s32 uniform_index = material.uniform_indices[material_uniform_index];
    s32 value_offset        = material.uniform_value_offsets[material_uniform_index];
    
    if (value_offset < 0) {
        value_offset = cache_uniform_value_on_cpu(uniform_index, data);
        material.uniform_value_offsets[material_uniform_index] = value_offset;
    } else {
        update_uniform_value_on_cpu(uniform_index, data, value_offset);
    }
}

u32 uniform_value_type_size(Uniform_Type type) {
    switch (type) {
	case UNIFORM_U32:     return 1 * sizeof(u32);
	case UNIFORM_F32:     return 1 * sizeof(f32);
	case UNIFORM_F32_2:   return 2 * sizeof(f32);
	case UNIFORM_F32_3:   return 3 * sizeof(f32);
	case UNIFORM_F32_4X4: return 4 * 4 * sizeof(f32);
    default:
        error("Failed to get uniform value size from type %d", type);
        return 0;
    }
}
