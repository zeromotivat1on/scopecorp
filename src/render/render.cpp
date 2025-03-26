#include "pch.h"
#include "render/viewport.h"
#include "render/render_registry.h"
#include "render/render_command.h"
#include "render/geometry_draw.h"
#include "render/text.h"

#include "game/world.h"
#include "game/entities.h"

#include "log.h"
#include "profile.h"
#include "memory_storage.h"
#include "font.h"

static s32 MAX_GEOMETRY_VERTEX_BUFFER_SIZE = 0;
static s32 GEOMETRY_VERTEX_DIMENSION       = 0;
static s32 GEOMETRY_VERTEX_SIZE            = 0;

static const char *vertex_region_begin_name   = "#begin vertex";
static const char *vertex_region_end_name     = "#end vertex";
static const char *fragment_region_begin_name = "#begin fragment";
static const char *fragment_region_end_name   = "#end fragment";

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
    list->entity       = create_shader(DIR_SHADERS "entity.glsl");
	list->text         = create_shader(DIR_SHADERS "text.glsl");
	list->skybox       = create_shader(DIR_SHADERS "skybox.glsl");
    list->frame_buffer = create_shader(DIR_SHADERS "frame_buffer.glsl");
    list->geometry     = create_shader(DIR_SHADERS "geometry.glsl");
    list->outline      = create_shader(DIR_SHADERS "outline.glsl");
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

void init_render_queue(Render_Queue *queue, s32 capacity) {
    assert(capacity <= MAX_RENDER_QUEUE_SIZE);
    
	queue->commands = push_array(pers, capacity, Render_Command);
	queue->size     = 0;
	queue->capacity = capacity;
}

void enqueue(Render_Queue *queue, const Render_Command *command) {
	assert(queue->size < MAX_RENDER_QUEUE_SIZE);
    
	const s32 index = queue->size++;
	memcpy(queue->commands + index, command, sizeof(Render_Command));
}

void flush(Render_Queue *queue) {
    PROFILE_SCOPE("Flush Render Queue");
    
	for (s32 i = 0; i < queue->size; ++i)
        submit(queue->commands + i);
    
	queue->size = 0;
}

void draw_world(const World *world) {
    PROFILE_SCOPE(__FUNCTION__);
    
	draw_entity(&world->skybox);

	for (s32 i = 0; i < world->static_meshes.count; ++i)
		draw_entity(world->static_meshes.items + i);

	draw_entity(&world->player);
}

void draw_entity(const Entity *e) {
    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR_TEST | RENDER_FLAG_CULL_FACE_TEST | RENDER_FLAG_BLEND_TEST | RENDER_FLAG_DEPTH_TEST | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_TRIANGLES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor_test.x      = viewport.x;
    command.scissor_test.y      = viewport.y;
    command.scissor_test.width  = viewport.width;
    command.scissor_test.height = viewport.height;
    command.cull_face_test.type    = CULL_FACE_BACK;
    command.cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command.blend_test.source      = BLEND_TEST_SOURCE_ALPHA;
    command.blend_test.destination = BLEND_TEST_ONE_MINUS_SOURCE_ALPHA;
    command.depth_test.function = DEPTH_TEST_LESS;
    command.depth_test.mask     = DEPTH_TEST_ENABLE;
    command.stencil_test.operation.stencil_failed = STENCIL_TEST_KEEP;
    command.stencil_test.operation.depth_failed   = STENCIL_TEST_KEEP;
    command.stencil_test.operation.both_passed    = STENCIL_TEST_REPLACE;
    command.stencil_test.function.type       = STENCIL_TEST_ALWAYS;
    command.stencil_test.function.comparator = 1;
    command.stencil_test.function.mask       = 0xFF;
    command.stencil_test.mask = 0x00;
    command.vertex_buffer_index = e->draw_data.vertex_buffer_index;
    command.index_buffer_index  = e->draw_data.index_buffer_index;
    fill_render_command_with_material_data(e->draw_data.material_index, &command);

    if (command.index_buffer_index != INVALID_INDEX) {
        const auto &index_buffer = render_registry.index_buffers[command.index_buffer_index];
        command.buffer_element_count = index_buffer.index_count;
    } else if (command.vertex_buffer_index != INVALID_INDEX) {
        const auto &vertex_buffer = render_registry.vertex_buffers[command.vertex_buffer_index];
        command.buffer_element_count = vertex_buffer.vertex_count;
    }
        
    if (e->flags & ENTITY_FLAG_WIREFRAME) {
        command.flags &= ~RENDER_FLAG_CULL_FACE_TEST;
        command.polygon_mode = POLYGON_LINE;
    }
    
    if (e->flags & ENTITY_FLAG_OUTLINE) {
        command.flags &= ~RENDER_FLAG_CULL_FACE_TEST;
        command.flags |= RENDER_FLAG_STENCIL_TEST;
        command.stencil_test.mask = 0xFF;
    }
    
	enqueue(&entity_render_queue, &command);

    if (e->flags & ENTITY_FLAG_OUTLINE) {        
        command.flags &= ~RENDER_FLAG_DEPTH_TEST;
        command.flags &= ~RENDER_FLAG_CULL_FACE_TEST;
        command.stencil_test.function.type       = STENCIL_TEST_NOT_EQUAL;
        command.stencil_test.function.comparator = 1;
        command.stencil_test.function.mask       = 0xFF;
        command.stencil_test.mask                = 0x00;

        const auto &material  = render_registry.materials[material_index_list.outline];
        command.shader_index  = material.shader_index;
        command.texture_index = material.texture_index;
        command.uniform_count = material.uniform_count;

        const Camera *camera = desired_camera(world);
        const mat4 view = camera_view(camera);
        const mat4 proj = camera_projection(camera);
        const mat4 mvp = mat4_transform(e->location, e->rotation, e->scale * 1.1f) * view * proj;
        set_material_uniform_value(material_index_list.outline, "u_transform", mvp.ptr());

        const vec3 color = vec3_yellow;
        set_material_uniform_value(material_index_list.outline, "u_color", &color);

        for (s32 i = 0; i < material.uniform_count; ++i) {
            command.uniform_indices[i]       = material.uniform_indices[i];
            command.uniform_value_offsets[i] = material.uniform_value_offsets[i];
        }
        
        enqueue(&entity_render_queue, &command);
    }
}

void init_geo_draw() {    
    const Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_3 };
    const s32 component_count = c_array_count(components);
    geometry_draw_buffer.vertex_buffer_index = create_vertex_buffer(components, component_count, null, MAX_GEOMETRY_VERTEX_COUNT, BUFFER_USAGE_STREAM);

	for (s32 i = 0; i < component_count; ++i) {
		GEOMETRY_VERTEX_DIMENSION += vertex_component_dimension(components[i]);
		GEOMETRY_VERTEX_SIZE      += vertex_component_size(components[i]);
    }
    
    MAX_GEOMETRY_VERTEX_BUFFER_SIZE = GEOMETRY_VERTEX_SIZE * MAX_GEOMETRY_VERTEX_COUNT;
    geometry_draw_buffer.vertex_data = push_array(pers, MAX_GEOMETRY_VERTEX_BUFFER_SIZE, f32);
        
    geometry_draw_buffer.material_index = create_material(shader_index_list.geometry, INVALID_INDEX);
    
    const s32 u_mvp = create_uniform("u_transform", UNIFORM_F32_4X4, 1);
    set_material_uniforms(geometry_draw_buffer.material_index, &u_mvp, 1);
}

void draw_geo_line(vec3 start, vec3 end, vec3 color) {
    assert(geometry_draw_buffer.vertex_count + 2 <= MAX_GEOMETRY_VERTEX_COUNT);
    
    const u32 offset = geometry_draw_buffer.vertex_count * GEOMETRY_VERTEX_DIMENSION;
    f32 *v = geometry_draw_buffer.vertex_data + offset;
    
    v[0] = start[0];
    v[1] = start[1];
    v[2] = start[2];

    v[3] = color[0];
    v[4] = color[1];
    v[5] = color[2];
    
    v[6] = end[0];
    v[7] = end[1];
    v[8] = end[2];

    v[9]  = color[0];
    v[10] = color[1];
    v[11] = color[2];

    geometry_draw_buffer.vertex_count += 2;
}

void draw_geo_box(vec3 points[8], vec3 color) {
    for (int i = 0; i < 4; ++i) {
        draw_geo_line(points[i],     points[(i + 1) % 4],       color);
        draw_geo_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        draw_geo_line(points[i],     points[i + 4],             color);
    }
}

void draw_geo_aabb(const AABB &aabb, vec3 color) {
    const vec3 bb[2] = { aabb.min, aabb.max };
    vec3 points[8];

    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    draw_geo_box(points, color);
}

void flush_geo_draw() {
    PROFILE_SCOPE("Flush Geometry Render Queue");
    
    if (geometry_draw_buffer.vertex_count == 0) return;
    
    Camera *camera = desired_camera(world);
	const mat4 view = camera_view(camera);
	const mat4 proj = camera_projection(camera);
    const mat4 vp = view * proj;
    set_material_uniform_value(geometry_draw_buffer.material_index, "u_transform", vp.ptr());
    
    set_vertex_buffer_data(geometry_draw_buffer.vertex_buffer_index, geometry_draw_buffer.vertex_data, geometry_draw_buffer.vertex_count * GEOMETRY_VERTEX_SIZE, 0);

    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR_TEST | RENDER_FLAG_CULL_FACE_TEST | RENDER_FLAG_DEPTH_TEST | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_LINES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor_test.x      = viewport.x;
    command.scissor_test.y      = viewport.y;
    command.scissor_test.width  = viewport.width;
    command.scissor_test.height = viewport.height;
    command.cull_face_test.type    = CULL_FACE_BACK;
    command.cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command.depth_test.function = DEPTH_TEST_LESS;
    command.depth_test.mask     = DEPTH_TEST_ENABLE;
    command.vertex_buffer_index = geometry_draw_buffer.vertex_buffer_index;
    command.buffer_element_count = geometry_draw_buffer.vertex_count;

    fill_render_command_with_material_data(geometry_draw_buffer.material_index, &command);
    submit(&command);
    
    geometry_draw_buffer.vertex_count = 0;
}

Text_Render_Command *create_default_text_draw_command(Font_Atlas *atlas) {
	// @Cleanup: create separate container for draw commands.
	auto *command = push_struct(pers, Text_Render_Command);
	*command = Text_Render_Command();

    command->flags = RENDER_FLAG_CULL_FACE_TEST | RENDER_FLAG_BLEND_TEST | RENDER_FLAG_DEPTH_TEST | RENDER_FLAG_RESET;
	command->render_mode  = RENDER_TRIANGLE_STRIP;
    command->polygon_mode = POLYGON_FILL;
    command->cull_face_test.type    = CULL_FACE_BACK;
    command->cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command->blend_test.source      = BLEND_TEST_SOURCE_ALPHA;
    command->blend_test.destination = BLEND_TEST_ONE_MINUS_SOURCE_ALPHA;
    command->depth_test.function = DEPTH_TEST_LESS;
    command->depth_test.mask     = DEPTH_TEST_DISABLE;

    const s32 material_index = material_index_list.text;
    set_material_uniform_value(material_index, "u_charmap",    command->charmap);
	set_material_uniform_value(material_index, "u_transforms", command->transforms);
	set_material_uniform_value(material_index, "u_projection", &command->projection);

    const vec3 default_color = vec3_white;
    set_material_uniform_value(material_index, "u_color", &default_color);

    fill_render_command_with_material_data(material_index, command);
    command->texture_index = atlas->texture_index;
	command->atlas = atlas;
    
	const f32 vertices[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};
	const Vertex_Component_Type components[] = { VERTEX_F32_2 };
	command->vertex_buffer_index = create_vertex_buffer(components, c_array_count(components), vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

    const auto &vertex_buffer = render_registry.vertex_buffers[command->vertex_buffer_index];
    command->buffer_element_count = vertex_buffer.vertex_count;
        
	return command;
}

void draw_text(Text_Render_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color) {
	const auto *atlas = command->atlas;

	set_material_uniform_value(material_index_list.text, "u_color", &color);

	s32 work_index = 0;
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < text_size; ++i) {
		const char c = text[i];

		if (c == ' ') {
			x += atlas->space_advance_width;
			continue;
		}

		if (c == '\t') {
			x += 4 * atlas->space_advance_width;
			continue;
		}

		if (c == '\n') {
			x = pos.x;
			y -= atlas->line_height;
			continue;
		}

		assert((u32)c >= atlas->start_charcode);
		assert((u32)c <= atlas->end_charcode);

		const u32 ci = c - atlas->start_charcode; // correctly shifted index
		const Font_Glyph_Metric *metric = atlas->metrics + ci;

		const f32 gw = (f32)atlas->font_size;
		const f32 gh = (f32)atlas->font_size;
		const f32 gx = x + metric->offset_x;
		const f32 gy = y - (gh + metric->offset_y);

		mat4 *transform = command->transforms + work_index;
		transform->identity().translate(vec3(gx, gy, 0.0f)).scale(vec3(gw, gh, 0.0f));

		command->charmap[work_index] = ci;

		if (++work_index >= TEXT_DRAW_BATCH_SIZE) {
            set_material_uniform_value(material_index_list.text, "u_charmap",    command->charmap);
            set_material_uniform_value(material_index_list.text, "u_transforms", command->transforms);
            
			command->instance_count = work_index;
			submit(command);
			work_index = 0;
		}

		x += metric->advance_width;
	}

	if (work_index > 0) {
        set_material_uniform_value(material_index_list.text, "u_charmap",    command->charmap);
        set_material_uniform_value(material_index_list.text, "u_transforms", command->transforms);
        
		command->instance_count = work_index;
		submit(command);
	}
}

void draw_text_with_shadow(Text_Render_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color) {
	draw_text(command, text, text_size, pos + shadow_offset, shadow_color);
	draw_text(command, text, text_size, pos, color);
}

void on_viewport_resize(Text_Render_Command *command, Viewport *viewport) {
	command->projection = mat4_orthographic(0, viewport->width, 0, viewport->height, -1, 1);
    set_material_uniform_value(material_index_list.text, "u_projection", &command->projection);
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

void fill_render_command_with_material_data(s32 material_index, Render_Command* command) {
    const auto &material = render_registry.materials[material_index];
    command->shader_index  = material.shader_index;
    command->texture_index = material.texture_index;
    command->uniform_count = material.uniform_count;
    
    for (s32 i = 0; i < material.uniform_count; ++i) {
        command->uniform_indices[i]       = material.uniform_indices[i];
        command->uniform_value_offsets[i] = material.uniform_value_offsets[i];
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

s32 vertex_component_dimension(Vertex_Component_Type type) {
	switch (type) {
	case VERTEX_S32:   return 1;
	case VERTEX_U32:   return 1;
	case VERTEX_F32_2: return 2;
	case VERTEX_F32_3: return 3;
	case VERTEX_F32_4: return 4;
	default:
		error("Failed to retreive dimension from unknown vertex attribute type %d", type);
		return -1;
	}
}

s32 vertex_component_size(Vertex_Component_Type type) {
	switch (type) {
    case VERTEX_S32:   return 1 * sizeof(s32);
    case VERTEX_U32:   return 1 * sizeof(u32);
	case VERTEX_F32_2: return 2 * sizeof(f32);
	case VERTEX_F32_3: return 3 * sizeof(f32);
	case VERTEX_F32_4: return 4 * sizeof(f32);
	default:
		error("Failed to retreive size from unknown vertex attribute type %d", type);
		return -1;
	}
}

s32 vertex_layout_dimension(const Vertex_Layout *layout) {
    s32 dimension = 0;
	for (s32 i = 0; i < layout->component_count; ++i)
		dimension += vertex_component_dimension(layout->components[i]);
    return dimension;
}

s32 vertex_layout_size(const Vertex_Layout *layout) {
    s32 size = 0;
	for (s32 i = 0; i < layout->component_count; ++i)
		size += vertex_component_size(layout->components[i]);
    return size;
}

static bool parse_shader_region(const char *shader_src, char *region_src, const char *region_begin_name, const char *region_end_name) {
    const char *region_begin = strstr(shader_src, region_begin_name);
    const char *region_end   = strstr(shader_src, region_end_name);

    if (!region_begin || !region_end) {
        error("Failed to find shader region %s ... %s", region_begin_name, region_end_name);
        return false;
    }

    region_begin += strlen(region_begin_name);

    const u64 region_size = region_end - region_begin;
    if (region_size >= MAX_SHADER_SIZE) {
        error("Shader region %s ... %s size is too big %u", region_begin_name, region_end_name, region_size);
        return false;
    }
    
    memcpy(region_src, region_begin, region_size);
	region_src[region_size] = '\0';

    return true;
}

bool parse_shader_source(const char *shader_src, char *vertex_src, char *fragment_src) {
    if (!parse_shader_region(shader_src, vertex_src, vertex_region_begin_name, vertex_region_end_name)) {
        error("Failed to parse vertex shader region");
        return false;
    }

    if (!parse_shader_region(shader_src, fragment_src, fragment_region_begin_name, fragment_region_end_name)) {
        error("Failed to parse fragment shader region");
        return false;
    }

    return true;
}
