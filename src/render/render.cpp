#include "pch.h"
#include "render/viewport.h"
#include "render/render_registry.h"
#include "render/render_command.h"
#include "render/render_stats.h"
#include "render/geometry_draw.h"
#include "render/text.h"

#include "game/world.h"
#include "game/entities.h"

#include "os/window.h"

#include "math/math_core.h"

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
	registry->vertex_arrays  = Sparse_Array<Vertex_Array>(MAX_VERTEX_ARRAYS);
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
    list->entity       = create_shader(SHADER_PATH("entity.glsl"));
    list->text         = create_shader(SHADER_PATH("text.glsl"));
    list->skybox       = create_shader(SHADER_PATH("skybox.glsl"));
    list->frame_buffer = create_shader(SHADER_PATH("frame_buffer.glsl"));
    list->geometry     = create_shader(SHADER_PATH("geometry.glsl"));
    list->outline      = create_shader(SHADER_PATH("outline.glsl"));
}

void load_game_textures(Texture_Index_List *list) {
	list->skybox = create_texture(TEXTURE_PATH("skybox.png"));
    list->stone  = create_texture(TEXTURE_PATH("stone.png"));
    list->grass  = create_texture(TEXTURE_PATH("grass.png"));

    list->player_idle[DIRECTION_BACK] = create_texture(TEXTURE_PATH("player_idle_back.png"));
    list->player_idle[DIRECTION_RIGHT] = create_texture(TEXTURE_PATH("player_idle_right.png"));
    list->player_idle[DIRECTION_LEFT] = create_texture(TEXTURE_PATH("player_idle_left.png"));
    list->player_idle[DIRECTION_FORWARD] = create_texture(TEXTURE_PATH("player_idle_forward.png"));

    list->player_move[DIRECTION_BACK][0] = create_texture(TEXTURE_PATH("player_move_back_1.png"));
    list->player_move[DIRECTION_BACK][1] = create_texture(TEXTURE_PATH("player_move_back_2.png"));
    list->player_move[DIRECTION_BACK][2] = create_texture(TEXTURE_PATH("player_move_back_3.png"));
    list->player_move[DIRECTION_BACK][3] = create_texture(TEXTURE_PATH("player_move_back_4.png"));

    list->player_move[DIRECTION_RIGHT][0] = create_texture(TEXTURE_PATH("player_move_right_1.png"));
    list->player_move[DIRECTION_RIGHT][1] = create_texture(TEXTURE_PATH("player_move_right_2.png"));
    list->player_move[DIRECTION_RIGHT][2] = create_texture(TEXTURE_PATH("player_move_right_3.png"));
    list->player_move[DIRECTION_RIGHT][3] = create_texture(TEXTURE_PATH("player_move_right_4.png"));

    list->player_move[DIRECTION_LEFT][0] = create_texture(TEXTURE_PATH("player_move_left_1.png"));
    list->player_move[DIRECTION_LEFT][1] = create_texture(TEXTURE_PATH("player_move_left_2.png"));
    list->player_move[DIRECTION_LEFT][2] = create_texture(TEXTURE_PATH("player_move_left_3.png"));
    list->player_move[DIRECTION_LEFT][3] = create_texture(TEXTURE_PATH("player_move_left_4.png"));

    list->player_move[DIRECTION_FORWARD][0] = create_texture(TEXTURE_PATH("player_move_forward_1.png"));
    list->player_move[DIRECTION_FORWARD][1] = create_texture(TEXTURE_PATH("player_move_forward_2.png"));
    list->player_move[DIRECTION_FORWARD][2] = create_texture(TEXTURE_PATH("player_move_forward_3.png"));
    list->player_move[DIRECTION_FORWARD][3] = create_texture(TEXTURE_PATH("player_move_forward_4.png"));
}

void create_game_materials(Material_Index_List *list) {        
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

	list->skybox  = create_material(shader_index_list.skybox,  texture_index_list.skybox);
    list->player  = create_material(shader_index_list.entity,  INVALID_INDEX);
	list->ground  = create_material(shader_index_list.entity,  texture_index_list.grass);
    list->cube    = create_material(shader_index_list.entity,  texture_index_list.stone);
    list->outline = create_material(shader_index_list.outline, INVALID_INDEX);
    
    set_material_uniforms(list->skybox,  skybox_uniforms,  COUNT(skybox_uniforms));
	set_material_uniforms(list->player,  entity_uniforms,  COUNT(entity_uniforms));
	set_material_uniforms(list->ground,  entity_uniforms,  COUNT(entity_uniforms));
    set_material_uniforms(list->cube,    entity_uniforms,  COUNT(entity_uniforms));
    set_material_uniforms(list->outline, outline_uniforms, COUNT(outline_uniforms));
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

void resize_viewport(Viewport *viewport, s16 width, s16 height) {
	switch (viewport->aspect_type) {
    case VIEWPORT_FILL_WINDOW:
        viewport->width = width;
		viewport->height = height;
        break;
	case VIEWPORT_4X3:
		viewport->width = width;
		viewport->height = height;

		if (width * 3 > height * 4) {
			viewport->width = height * 4 / 3;
			viewport->x = (width - viewport->width) / 2;
		} else {
			viewport->height = width * 3 / 4;
			viewport->y = (height - viewport->height) / 2;
		}

		break;
	default:
		error("Failed to resize viewport with unknown aspect type %d", viewport->aspect_type);
		break;
	}


    if (viewport->frame_buffer_index != INVALID_INDEX) {
        const s16 width  = (s16)((f32)viewport->width  * viewport->resolution_scale);
        const s16 height = (s16)((f32)viewport->height * viewport->resolution_scale);
    
        recreate_frame_buffer(viewport->frame_buffer_index, width, height);
        log("Recreated viewport frame buffer %dx%d", width, height);
    }
}

void draw_world(const World *world) {
    PROFILE_SCOPE(__FUNCTION__);
    
	draw_entity(&world->skybox);

	for (s32 i = 0; i < world->static_meshes.count; ++i)
		draw_entity(world->static_meshes.items + i);

	draw_entity(&world->player);
}

void draw_entity(const Entity *e) {
    const auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];

    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_BLEND | RENDER_FLAG_DEPTH | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_TRIANGLES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor.x      = 0;
    command.scissor.y      = 0;
    command.scissor.width  = viewport_frame_buffer.width;
    command.scissor.height = viewport_frame_buffer.height;
    command.cull_face.type    = CULL_FACE_BACK;
    command.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
    command.blend.source      = BLEND_SOURCE_ALPHA;
    command.blend.destination = BLEND_ONE_MINUS_SOURCE_ALPHA;
    command.depth.function = DEPTH_LESS;
    command.depth.mask     = DEPTH_ENABLE;
    command.stencil.operation.stencil_failed = STENCIL_KEEP;
    command.stencil.operation.depth_failed   = STENCIL_KEEP;
    command.stencil.operation.both_passed    = STENCIL_REPLACE;
    command.stencil.function.type       = STENCIL_ALWAYS;
    command.stencil.function.comparator = 1;
    command.stencil.function.mask       = 0xFF;
    command.stencil.mask = 0x00;
    command.vertex_array_index = e->draw_data.vertex_array_index;
    command.index_buffer_index = e->draw_data.index_buffer_index;

    fill_render_command_with_material_data(e->draw_data.material_index, &command);

    if (command.index_buffer_index != INVALID_INDEX) {
        const auto &index_buffer = render_registry.index_buffers[command.index_buffer_index];
        command.buffer_element_count = index_buffer.index_count;
    } else if (command.vertex_array_index != INVALID_INDEX) {
        command.buffer_element_count = vertex_array_vertex_count(command.vertex_array_index);
    }
    
    if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) {
        command.flags &= ~RENDER_FLAG_CULL_FACE;
        command.flags |= RENDER_FLAG_STENCIL;
        command.stencil.mask = 0xFF;

        draw_geo_cross(e->location, 0.5f);
    }
    
	enqueue(&entity_render_queue, &command);

    if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) { // draw outline
        command.flags &= ~RENDER_FLAG_DEPTH;
        command.flags &= ~RENDER_FLAG_CULL_FACE;
        command.stencil.function.type       = STENCIL_NOT_EQUAL;
        command.stencil.function.comparator = 1;
        command.stencil.function.mask       = 0xFF;
        command.stencil.mask                = 0x00;

        const vec3 color = vec3_yellow;
        const mat4 mvp = mat4_transform(e->location, e->rotation, e->scale * 1.1f) * world->camera_view_proj;

        set_material_uniform_value(material_index_list.outline, "u_color", &color);
        set_material_uniform_value(material_index_list.outline, "u_transform", mvp.ptr());

        fill_render_command_with_material_data(material_index_list.outline, &command);
        
        enqueue(&entity_render_queue, &command);
    }
}

void init_geo_draw() {    
    Vertex_Array_Binding binding = {};
    binding.layout_size = 2;
    binding.layout[0] = { VERTEX_F32_3, 0 };
    binding.layout[1] = { VERTEX_F32_3, 0 };

    for (s32 i = 0; i < binding.layout_size; ++i) {
		GEOMETRY_VERTEX_DIMENSION += vertex_component_dimension(binding.layout[i].type);
		GEOMETRY_VERTEX_SIZE      += vertex_component_size(binding.layout[i].type);
    }
    
    MAX_GEOMETRY_VERTEX_BUFFER_SIZE = GEOMETRY_VERTEX_SIZE * MAX_GEOMETRY_VERTEX_COUNT;
    geometry_draw_buffer.vertex_data = push_array(pers, MAX_GEOMETRY_VERTEX_BUFFER_SIZE, f32);

    binding.vertex_buffer_index = create_vertex_buffer(null, MAX_GEOMETRY_VERTEX_BUFFER_SIZE, BUFFER_USAGE_STREAM);
    
    geometry_draw_buffer.vertex_array_index = create_vertex_array(&binding, 1);
    geometry_draw_buffer.material_index = create_material(shader_index_list.geometry, INVALID_INDEX);

    const s32 u_transform = create_uniform("u_transform", UNIFORM_F32_4X4, 1);
    set_material_uniforms(geometry_draw_buffer.material_index, &u_transform, 1);
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

void draw_geo_arrow(vec3 start, vec3 end, vec3 color) {
    static const f32 size = 0.04f;
    static const f32 arrow_step = 30.0f; // In degrees
    static const f32 arrow_sin[45] = {
        0.0f, 0.5f, 0.866025f, 1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866025f,
        -1.0f, -0.866025f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    static const f32 arrow_cos[45] = {
        1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866026f, -1.0f, -0.866025f, -0.5f, 0.0f,
        0.5f, 0.866026f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    draw_geo_line(start, end, color);

    vec3 forward = normalize(end - start);
	const vec3 right = absf(forward.y) > 1.0f - F32_EPSILON
        ? normalize(vec3_right.cross(forward))
        : normalize(vec3_up.cross(forward));
	const vec3 up = forward.cross(right);
    forward *= size;
    
    f32 degrees = 0.0f;
    for (int i = 0; degrees < 360.0f; degrees += arrow_step, ++i) {
        f32 scale;
        vec3 v1, v2;

        scale = 0.5f * size * arrow_cos[i];
        v1 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i];
        v1 += up * scale;

        scale = 0.5f * size * arrow_cos[i + 1];
        v2 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i + 1];
        v2 += up * scale;

        draw_geo_line(v1, end, color);
        draw_geo_line(v1, v2,  color);
    }
}

void draw_geo_cross(vec3 location, f32 size) {
    draw_geo_arrow(location, location + vec3_up * size,      vec3_blue);
    draw_geo_arrow(location, location + vec3_right * size,   vec3_green);
    draw_geo_arrow(location, location + vec3_forward * size, vec3_red);
}

void draw_geo_box(const vec3 points[8], vec3 color) {
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
    PROFILE_SCOPE(__FUNCTION__);
    
    if (geometry_draw_buffer.vertex_count == 0) return;

    const auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];
        
    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_LINES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor.x      = 0;
    command.scissor.y      = 0;
    command.scissor.width  = viewport_frame_buffer.width;
    command.scissor.height = viewport_frame_buffer.height;
    command.cull_face.type    = CULL_FACE_BACK;
    command.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
    command.depth.function = DEPTH_LESS;
    command.depth.mask     = DEPTH_ENABLE;
    command.vertex_array_index  = geometry_draw_buffer.vertex_array_index;
    command.buffer_element_count = geometry_draw_buffer.vertex_count;

    fill_render_command_with_material_data(geometry_draw_buffer.material_index, &command);

    const auto &vertex_array = render_registry.vertex_arrays[command.vertex_array_index]; 
    set_vertex_buffer_data(vertex_array.bindings[0].vertex_buffer_index,
                           geometry_draw_buffer.vertex_data,
                           geometry_draw_buffer.vertex_count * GEOMETRY_VERTEX_SIZE, 0);

    Camera *camera = desired_camera(world);
	const mat4 view = camera_view(camera);
	const mat4 proj = camera_projection(camera);
    const mat4 vp = view * proj;
    set_material_uniform_value(geometry_draw_buffer.material_index, "u_transform", vp.ptr());

    submit(&command);
    
    geometry_draw_buffer.vertex_count = 0;
}

void draw_geo_debug() {
    const auto &player = world->player;
    const vec3 player_center_location = player.location + vec3(0.0f, player.scale.y * 0.5f, 0.0f);
    draw_geo_arrow(player_center_location, player_center_location + normalize(player.velocity) * 0.5f, vec3_red);

    if (player.collide_aabb_index != INVALID_INDEX) {
        draw_geo_aabb(world->aabbs[player.aabb_index],         vec3_green);
        draw_geo_aabb(world->aabbs[player.collide_aabb_index], vec3_green);
    }
}

void init_text_draw(Font_Atlas *atlas) {
    const s32 color_buffer_size     = MAX_CHAR_RENDER_COUNT * sizeof(vec3);
    const s32 charmap_buffer_size   = MAX_CHAR_RENDER_COUNT * sizeof(u32);
    const s32 transform_buffer_size = MAX_CHAR_RENDER_COUNT * sizeof(mat4);

    text_draw_buffer.atlas = atlas;
    text_draw_buffer.colors     = (vec3 *)push(pers, color_buffer_size);
    text_draw_buffer.charmap    = (u32  *)push(pers, charmap_buffer_size);
    text_draw_buffer.transforms = (mat4 *)push(pers, transform_buffer_size);
    
    Vertex_Array_Binding bindings[4] = {};

    const f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, };
    bindings[0].layout_size = 1;
    bindings[0].layout[0] = { VERTEX_F32_2, 0 };
    bindings[0].vertex_buffer_index = create_vertex_buffer(vertices, 8 * sizeof(f32), BUFFER_USAGE_STATIC);
    
    bindings[1].layout_size = 1;
    bindings[1].layout[0] = { VERTEX_F32_3, 1 };
    bindings[1].vertex_buffer_index = create_vertex_buffer(null, color_buffer_size, BUFFER_USAGE_STREAM);
 
    bindings[2].layout_size = 1;
    bindings[2].layout[0] = { VERTEX_U32, 1 };
    bindings[2].vertex_buffer_index = create_vertex_buffer(null, charmap_buffer_size, BUFFER_USAGE_STREAM);
    
    bindings[3].layout_size = 4;
    bindings[3].layout[0] = { VERTEX_F32_4, 1 };
    bindings[3].layout[1] = { VERTEX_F32_4, 1 };
    bindings[3].layout[2] = { VERTEX_F32_4, 1 };
    bindings[3].layout[3] = { VERTEX_F32_4, 1 };
    bindings[3].vertex_buffer_index = create_vertex_buffer(null, transform_buffer_size, BUFFER_USAGE_STREAM);

    text_draw_buffer.vertex_array_index = create_vertex_array(bindings, 4);
    text_draw_buffer.material_index = create_material(shader_index_list.text, INVALID_INDEX);

    const s32 u_projection = create_uniform("u_projection", UNIFORM_F32_4X4, 1);
    set_material_uniforms(text_draw_buffer.material_index, &u_projection, 1);
}

void draw_text(const char *text, u32 text_size, vec2 pos, vec3 color) {
	const auto *atlas = text_draw_buffer.atlas;

	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < text_size; ++i) {
        assert(text_draw_buffer.char_count < MAX_CHAR_RENDER_COUNT);
        
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

		const u32 ci = c - atlas->start_charcode;
		const Font_Glyph_Metric *metric = atlas->metrics + ci;

		const f32 cw = (f32)atlas->font_size;
		const f32 ch = (f32)atlas->font_size;
		const f32 cx = x + metric->offset_x;
		const f32 cy = y - (ch + metric->offset_y);

        const vec3 location  = vec3(cx, cy, 0.0f);
        const vec3 scale     = vec3(cw, ch, 0.0f);
        const mat4 transform = mat4_identity().translate(location).scale(scale);

        text_draw_buffer.colors[text_draw_buffer.char_count] = color;
        text_draw_buffer.charmap[text_draw_buffer.char_count] = ci;
		text_draw_buffer.transforms[text_draw_buffer.char_count] = transform;
        text_draw_buffer.char_count++;
        
		x += metric->advance_width;
	}
}

void draw_text_with_shadow(const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color) {
	draw_text(text, text_size, pos + shadow_offset, shadow_color);
	draw_text(text, text_size, pos, color);
}

void flush_text_draw() {
    PROFILE_SCOPE(__FUNCTION__);

    if (text_draw_buffer.char_count == 0) return;

    const auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];
        
    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_BLEND | RENDER_FLAG_DEPTH | RENDER_FLAG_RESET;
	command.render_mode  = RENDER_TRIANGLE_STRIP;
    command.polygon_mode = POLYGON_FILL;
    command.scissor.x      = 0;
    command.scissor.y      = 0;
    command.scissor.width  = viewport_frame_buffer.width;
    command.scissor.height = viewport_frame_buffer.height;
    command.cull_face.type    = CULL_FACE_BACK;
    command.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
    command.blend.source      = BLEND_SOURCE_ALPHA;
    command.blend.destination = BLEND_ONE_MINUS_SOURCE_ALPHA;
    command.depth.function = DEPTH_LESS;
    command.depth.mask     = DEPTH_DISABLE;
    command.vertex_array_index = text_draw_buffer.vertex_array_index;
    command.buffer_element_count = 4;
    command.instance_count       = text_draw_buffer.char_count;
    
    fill_render_command_with_material_data(text_draw_buffer.material_index, &command);
    command.texture_index = text_draw_buffer.atlas->texture_index;

    const auto &vertex_array = render_registry.vertex_arrays[text_draw_buffer.vertex_array_index];
    set_vertex_buffer_data(vertex_array.bindings[1].vertex_buffer_index,
                           text_draw_buffer.colors,
                           text_draw_buffer.char_count * sizeof(vec3), 0);
    set_vertex_buffer_data(vertex_array.bindings[2].vertex_buffer_index,
                           text_draw_buffer.charmap,
                           text_draw_buffer.char_count * sizeof(u32), 0);
    set_vertex_buffer_data(vertex_array.bindings[3].vertex_buffer_index,
                           text_draw_buffer.transforms,
                           text_draw_buffer.char_count * sizeof(mat4), 0);
        
    submit(&command);

    text_draw_buffer.char_count = 0;
}

s32 vertex_array_vertex_count(s32 vertex_array_index) {
    const auto &vertex_array = render_registry.vertex_arrays[vertex_array_index];

    s32 vertex_count = 0;
    for (s32 i = 0; i < vertex_array.binding_count; ++i) {
        const auto &binding = vertex_array.bindings[i];
        const auto &vertex_buffer = render_registry.vertex_buffers[binding.vertex_buffer_index];
        s32 vertex_size = 0;
        for (s32 j = 0; j < binding.layout_size; ++j)
            vertex_size += vertex_component_size(binding.layout[j].type);

        vertex_count += vertex_buffer.size / vertex_size;
    }
    
    return vertex_count;
}

s32 create_uniform(const char *name, Uniform_Type type, s32 element_count) {
    Uniform uniform;
    uniform.name  = name;
    uniform.type  = type;
    uniform.count = element_count;

    return render_registry.uniforms.add(uniform);
}

u32 cache_uniform_value_on_cpu(s32 uniform_index, const void *data) {
    assert(data);
    
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
    auto &material = render_registry.materials[material_index];
    assert(material.uniform_count == 0);

    material.uniform_count = count;
    memcpy(material.uniform_indices, uniform_indices, count * sizeof(s32));

    auto &cache = render_registry.uniform_value_cache;
    
    for (s32 i = 0; i < material.uniform_count; ++i) {
        const auto &uniform = render_registry.uniforms[material.uniform_indices[i]];
        const u32 uniform_data_size = uniform.count * uniform_value_type_size(uniform.type);

        const u32 offset = cache.size;
        cache.size += uniform_data_size;
        
        material.uniform_value_offsets[i] = offset;
    }
}

void set_material_uniform_value(s32 material_index, s32 material_uniform_index, const void *data) {
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

void set_material_uniform_value(s32 material_index, const char *name, const void *data) {
	const s32 material_uniform_index = find_material_uniform(material_index, name);
	if (material_uniform_index == INVALID_INDEX) {
		error("Failed to set material uniform %s value as its not found", name);
		return;
	}

    set_material_uniform_value(material_index, material_uniform_index, data);
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

void update_render_stats() {
    constexpr s32 dt_frame_count = 512;
    static f32 previous_dt_table[dt_frame_count];
    
    frame_index++;
    previous_dt_table[frame_index % dt_frame_count] = delta_time;

    if (frame_index % dt_frame_count == 0) {
        f32 dt_sum  = 0.0f;
        for (s32 i = 0; i < dt_frame_count; ++i)
            dt_sum  += previous_dt_table[i];
        
        average_dt  = dt_sum / dt_frame_count;
        average_fps = 1.0f / average_dt;
    }
    
    draw_call_count = entity_render_queue.size +
        (s32)(geometry_draw_buffer.vertex_count > 0) +
        (s32)(text_draw_buffer.char_count > 0);
}
