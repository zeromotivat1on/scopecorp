#include "pch.h"
#include "log.h"
#include "sid.h"
#include "str.h"
#include "font.h"
#include "profile.h"
#include "asset.h"

#include "render/viewport.h"
#include "render/render_command.h"
#include "render/render_stats.h"
#include "render/buffer_storage.h"
#include "render/geometry.h"
#include "render/texture.h"
#include "render/material.h"
#include "render/uniform.h"
#include "render/frame_buffer.h"
#include "render/mesh.h"
#include "render/ui.h"

#include "game/game.h"
#include "game/world.h"
#include "game/entity.h"

#include "os/atomic.h"
#include "os/window.h"

#include "math/math_core.h"

void cache_texture_sids(Texture_Sid_List *list) {
    list->skybox = SID("/data/textures/skybox.png");
    list->stone  = SID("/data/textures/stone.png");
    list->grass  = SID("/data/textures/grass.png");

    list->player_idle[DIRECTION_BACK]    = SID("/data/textures/player_idle_back.png");
    list->player_idle[DIRECTION_RIGHT]   = SID("/data/textures/player_idle_right.png");
    list->player_idle[DIRECTION_LEFT]    = SID("/data/textures/player_idle_left.png");
    list->player_idle[DIRECTION_FORWARD] = SID("/data/textures/player_idle_forward.png");

    list->player_move[DIRECTION_BACK][0] = SID("/data/textures/player_move_back_1.png");
    list->player_move[DIRECTION_BACK][1] = SID("/data/textures/player_move_back_2.png");
    list->player_move[DIRECTION_BACK][2] = SID("/data/textures/player_move_back_3.png");
    list->player_move[DIRECTION_BACK][3] = SID("/data/textures/player_move_back_4.png");

    list->player_move[DIRECTION_RIGHT][0] = SID("/data/textures/player_move_right_1.png");
    list->player_move[DIRECTION_RIGHT][1] = SID("/data/textures/player_move_right_2.png");
    list->player_move[DIRECTION_RIGHT][2] = SID("/data/textures/player_move_right_3.png");
    list->player_move[DIRECTION_RIGHT][3] = SID("/data/textures/player_move_right_4.png");

    list->player_move[DIRECTION_LEFT][0] = SID("/data/textures/player_move_left_1.png");
    list->player_move[DIRECTION_LEFT][1] = SID("/data/textures/player_move_left_2.png");
    list->player_move[DIRECTION_LEFT][2] = SID("/data/textures/player_move_left_3.png");
    list->player_move[DIRECTION_LEFT][3] = SID("/data/textures/player_move_left_4.png");

    list->player_move[DIRECTION_FORWARD][0] = SID("/data/textures/player_move_forward_1.png");
    list->player_move[DIRECTION_FORWARD][1] = SID("/data/textures/player_move_forward_2.png");
    list->player_move[DIRECTION_FORWARD][2] = SID("/data/textures/player_move_forward_3.png");
    list->player_move[DIRECTION_FORWARD][3] = SID("/data/textures/player_move_forward_4.png");
}

void init_render_queue(Render_Queue *queue, s32 capacity) {
    Assert(capacity <= MAX_RENDER_QUEUE_SIZE);
    
	queue->commands = allocltn(Render_Command, capacity);
	queue->size     = 0;
	queue->capacity = capacity;
}

void r_enqueue(Render_Queue *queue, const Render_Command *command) {
	Assert(queue->size < MAX_RENDER_QUEUE_SIZE);
    
	const s32 index = queue->size++;
	copy_bytes(queue->commands + index, command, sizeof(Render_Command));
}

void r_flush(Render_Queue *queue) {
    PROFILE_SCOPE("flush_render_queue");
    
    for (s32 i = 0; i < queue->size; ++i) {
        r_submit(queue->commands + i);
    }

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


    const s16 fb_width  = (s16)((f32)viewport->width  * viewport->resolution_scale);
    const s16 fb_height = (s16)((f32)viewport->height * viewport->resolution_scale);
    
    r_recreate_frame_buffer(&viewport->frame_buffer, fb_width, fb_height);
    log("Recreated viewport frame buffer %dx%d", fb_width, fb_height);
}

void draw_world(const World *world) {
    PROFILE_SCOPE(__FUNCTION__);
    
	draw_entity(&world->skybox);

	For (world->static_meshes) {
		draw_entity(&it);
    }
    
	draw_entity(&world->player);
}

void draw_entity(const Entity *e) {
    const auto &frame_buffer = viewport.frame_buffer;

    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_BLEND | RENDER_FLAG_DEPTH | RENDER_FLAG_INDEXED | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_TRIANGLES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor.x      = 0;
    command.scissor.y      = 0;
    command.scissor.width  = frame_buffer.width;
    command.scissor.height = frame_buffer.height;
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

    const auto &mesh = asset_table.meshes[e->draw_data.sid_mesh];
    command.rid_vertex_array = mesh.rid_vertex_array;
    command.buffer_element_count = mesh.index_count;
    command.buffer_element_offset = mesh.index_data_offset;
    
    command.sid_material = e->draw_data.sid_material;
    
    if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) {
        command.flags &= ~RENDER_FLAG_CULL_FACE;
        command.flags |= RENDER_FLAG_STENCIL;
        command.stencil.mask = 0xFF;

        geo_draw_cross(e->location, 0.5f);
    }

    command.eid_vertex_data_offset = e->draw_data.eid_vertex_data_offset;
    
	r_enqueue(&entity_render_queue, &command);

    if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) { // draw outline
        command.flags &= ~RENDER_FLAG_DEPTH;
        command.flags &= ~RENDER_FLAG_CULL_FACE;
        command.stencil.function.type       = STENCIL_NOT_EQUAL;
        command.stencil.function.comparator = 1;
        command.stencil.function.mask       = 0xFF;
        command.stencil.mask                = 0x00;

        const auto *camera = desired_camera(world);
        const u32 color = rgba_yellow;
        const mat4 mvp = mat4_transform(e->location, e->rotation, e->scale * 1.1f) * camera->view_proj;

        auto &material = asset_table.materials[SID_MATERIAL_OUTLINE];
        set_material_uniform_value(&material, "u_color",     &color, sizeof(color));
        set_material_uniform_value(&material, "u_transform", &mvp,   sizeof(mvp));

        command.sid_material = SID_MATERIAL_OUTLINE;
        
        r_enqueue(&entity_render_queue, &command);
    }
}

void geo_init() {    
    constexpr u32 location_buffer_size = MAX_GEOMETRY_VERTEX_COUNT * sizeof(vec3);
    constexpr u32 color_buffer_size    = MAX_GEOMETRY_VERTEX_COUNT * sizeof(u32);

    const u32 location_buffer_offset  = vertex_buffer_storage.size;
    const u32 color_buffer_offset     = location_buffer_offset + location_buffer_size;

    auto &gdb = geo_draw_buffer;
    gdb.locations  = (vec3 *)r_allocv(location_buffer_size); 
    gdb.colors     = (u32  *)r_allocv(color_buffer_size);
    gdb.vertex_count = 0;
    
    Vertex_Array_Binding bindings[2] = {};
    bindings[0].binding_index = 0;
    bindings[0].data_offset = location_buffer_offset;
    bindings[0].layout_size = 1;
    bindings[0].layout[0] = { VERTEX_F32_3, 0 };

    bindings[1].binding_index = 1;
    bindings[1].data_offset = color_buffer_offset;
    bindings[1].layout_size = 1;
    bindings[1].layout[0] = { VERTEX_U32, 0 };
    
    gdb.rid_vertex_array = r_create_vertex_array(bindings, COUNT(bindings));
    gdb.sid_material = SID_MATERIAL_GEOMETRY;
}

void geo_draw_line(vec3 start, vec3 end, u32 color) {
    Assert(geo_draw_buffer.vertex_count + 2 <= MAX_GEOMETRY_VERTEX_COUNT);

    auto &gdb = geo_draw_buffer;

    vec3 *vl = gdb.locations + gdb.vertex_count;
    u32  *vc = gdb.colors    + gdb.vertex_count;
        
    vl[0] = start;
    vl[1] = end;
    vc[0] = color;
    vc[1] = color;

    gdb.vertex_count += 2;
}

void geo_draw_arrow(vec3 start, vec3 end, u32 color) {
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

    geo_draw_line(start, end, color);

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

        geo_draw_line(v1, end, color);
        geo_draw_line(v1, v2,  color);
    }
}

void geo_draw_cross(vec3 location, f32 size) {
    geo_draw_arrow(location, location + vec3_up * size,      rgba_blue);
    geo_draw_arrow(location, location + vec3_right * size,   rgba_green);
    geo_draw_arrow(location, location + vec3_forward * size, rgba_red);
}

void geo_draw_box(const vec3 points[8], u32 color) {
    for (int i = 0; i < 4; ++i) {
        geo_draw_line(points[i],     points[(i + 1) % 4],       color);
        geo_draw_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        geo_draw_line(points[i],     points[i + 4],             color);
    }
}

void geo_draw_aabb(const AABB &aabb, u32 color) {
    const vec3 bb[2] = { aabb.min, aabb.max };
    vec3 points[8];

    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    geo_draw_box(points, color);
}

void geo_flush() {
    PROFILE_SCOPE(__FUNCTION__);

    auto &gdb = geo_draw_buffer;
    
    if (gdb.vertex_count == 0) return;

    const auto &frame_buffer = viewport.frame_buffer;
        
    Render_Command command = {};
    command.flags = RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_RESET;
    command.render_mode  = RENDER_LINES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor.x      = 0;
    command.scissor.y      = 0;
    command.scissor.width  = frame_buffer.width;
    command.scissor.height = frame_buffer.height;
    command.cull_face.type    = CULL_FACE_BACK;
    command.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
    command.depth.function = DEPTH_LESS;
    command.depth.mask     = DEPTH_ENABLE;

    command.rid_vertex_array = gdb.rid_vertex_array;
    command.sid_material = gdb.sid_material;
    command.buffer_element_count  = gdb.vertex_count;
    command.buffer_element_offset = 0;
    command.instance_count  = 1;
    command.instance_offset = 0;
    
    r_submit(&command);
    
    gdb.vertex_count = 0;
}

void ui_init() {
    {
        Font_Info *font_info = create_font_info(asset_table.fonts[SID_FONT_BETTER_VCR].data);
        Font_Atlas *atlas_16 = bake_font_atlas(font_info, 33, 126, 16);
        Font_Atlas *atlas_24 = bake_font_atlas(font_info, 33, 126, 24);
        
        ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX] = atlas_16;
        ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX] = atlas_16;
        ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX] = atlas_16;
        ui.font_atlases[UI_SCREEN_REPORT_FONT_ATLAS_INDEX] = atlas_24;
    }
    
    {   // Text draw buffer.
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        constexpr u32 position_buffer_size  = sizeof(vertices);
        constexpr u32 color_buffer_size     = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(u32);
        constexpr u32 charmap_buffer_size   = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(u32);
        constexpr u32 transform_buffer_size = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(mat4);

        const u32 position_buffer_offset  = vertex_buffer_storage.size;
        const u32 color_buffer_offset     = position_buffer_offset + position_buffer_size;
        const u32 charmap_buffer_offset   = color_buffer_offset + color_buffer_size;
        const u32 transform_buffer_offset = charmap_buffer_offset + charmap_buffer_size;

        auto &tdb = ui.text_draw_buffer;
        tdb.positions  = (f32  *)r_allocv(position_buffer_size); 
        tdb.colors     = (u32  *)r_allocv(color_buffer_size);
        tdb.charmap    = (u32  *)r_allocv(charmap_buffer_size);
        tdb.transforms = (mat4 *)r_allocv(transform_buffer_size);

        copy_bytes(tdb.positions, vertices, position_buffer_size);
        
        Vertex_Array_Binding bindings[4] = {};
        bindings[0].binding_index = 0;
        bindings[0].data_offset = position_buffer_offset;
        bindings[0].layout_size = 1;
        bindings[0].layout[0] = { VERTEX_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].data_offset = color_buffer_offset;
        bindings[1].layout_size = 1;
        bindings[1].layout[0] = { VERTEX_U32, 1 };

        bindings[2].binding_index = 2;
        bindings[2].data_offset = charmap_buffer_offset;
        bindings[2].layout_size = 1;
        bindings[2].layout[0] = { VERTEX_U32, 1 };

        bindings[3].binding_index = 3;
        bindings[3].data_offset = transform_buffer_offset;
        bindings[3].layout_size = 4;
        bindings[3].layout[0] = { VERTEX_F32_4, 1 };
        bindings[3].layout[1] = { VERTEX_F32_4, 1 };
        bindings[3].layout[2] = { VERTEX_F32_4, 1 };
        bindings[3].layout[3] = { VERTEX_F32_4, 1 };

        tdb.rid_vertex_array = r_create_vertex_array(bindings, COUNT(bindings));
        tdb.sid_material = SID_MATERIAL_UI_TEXT;
    }

    {   // Quad draw buffer.
        constexpr u32 pos_buffer_size   = 4 * MAX_UI_QUAD_DRAW_BUFFER_QUADS * sizeof(vec2);
        constexpr u32 color_buffer_size = 1 * MAX_UI_QUAD_DRAW_BUFFER_QUADS * sizeof(u32);

        const u32 pos_buffer_offset   = vertex_buffer_storage.size;
        const u32 color_buffer_offset = pos_buffer_offset + pos_buffer_size;
        
        auto &qdb = ui.quad_draw_buffer;
        qdb.positions = (vec2 *)r_allocv(pos_buffer_size);
        qdb.colors    = (u32  *)r_allocv(color_buffer_size);
        
        Vertex_Array_Binding bindings[2] = {};
        bindings[0].binding_index = 0;
        bindings[0].data_offset = pos_buffer_offset;
        bindings[0].layout_size = 1;
        bindings[0].layout[0] = { VERTEX_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].data_offset = color_buffer_offset;
        bindings[1].layout_size = 1;
        bindings[1].layout[0] = { VERTEX_U32, 0 };
        
        qdb.rid_vertex_array = r_create_vertex_array(bindings, COUNT(bindings));
        qdb.sid_material = SID_MATERIAL_UI_ELEMENT;
    }
}

void ui_draw_text(const char *text, vec2 pos, u32 color, s32 atlas_index) {
    ui_draw_text(text, (u32)str_size(text), pos, color, atlas_index);
}

void ui_draw_text(const char *text, u32 count, vec2 pos, u32 color, s32 atlas_index) {
    Assert(atlas_index < MAX_UI_FONT_ATLASES);
    Assert(ui.draw_queue_size < MAX_UI_DRAW_QUEUE_SIZE);

	const auto &atlas = *ui.font_atlases[atlas_index];
    auto &tdb = ui.text_draw_buffer;

    auto &ui_cmd = ui.draw_queue[ui.draw_queue_size];
    ui_cmd.type = UI_DRAW_TEXT;
    ui_cmd.element_count = 0;
    ui_cmd.atlas_index = atlas_index;
    
    ui.draw_queue_size += 1;
    
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < count; ++i) {
        Assert(tdb.char_count < MAX_UI_TEXT_DRAW_BUFFER_CHARS);
        
		const char c = text[i];

		if (c == ' ') {
			x += atlas.space_advance_width;
			continue;
		}

		if (c == '\t') {
			x += 4 * atlas.space_advance_width;
			continue;
		}

		if (c == '\n') {
			x = pos.x;
			y -= atlas.line_height;
			continue;
		}

		Assert((u32)c >= atlas.start_charcode);
		Assert((u32)c <= atlas.end_charcode);

		const u32 ci = c - atlas.start_charcode;
		const Font_Glyph_Metric *metric = atlas.metrics + ci;

		const f32 cw = (f32)atlas.font_size;
		const f32 ch = (f32)atlas.font_size;
		const f32 cx = x + metric->offset_x;
		const f32 cy = y - (ch + metric->offset_y);

        const vec3 location  = vec3(cx, cy, 0.0f);
        const vec3 scale     = vec3(cw, ch, 0.0f);
        const mat4 transform = mat4_identity().translate(location).scale(scale);

        tdb.colors[tdb.char_count] = color;
        tdb.charmap[tdb.char_count] = ci;
		tdb.transforms[tdb.char_count] = transform;
        tdb.char_count += 1;

        ui_cmd.element_count += 1;
        
		x += metric->advance_width;
        if (i < count - 1) {
            x += atlas.px_h_scale * get_glyph_kern_advance(atlas.font, c, text[i + 1]);
        }
	}
}

void ui_draw_text_with_shadow(const char *text, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index) {
    ui_draw_text_with_shadow(text, (u32)str_size(text), pos, color, shadow_offset, shadow_color, atlas_index);
}

void ui_draw_text_with_shadow(const char *text, u32 count, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index) {
	ui_draw_text(text, count, pos + shadow_offset, shadow_color, atlas_index);
	ui_draw_text(text, count, pos, color, atlas_index);
}

void ui_draw_quad(vec2 p0, vec2 p1, u32 color) {
    Assert(ui.draw_queue_size < MAX_UI_DRAW_QUEUE_SIZE);

    auto &ui_cmd = ui.draw_queue[ui.draw_queue_size];
    ui_cmd.type = UI_DRAW_QUAD;
    ui_cmd.element_count = 1;

    ui.draw_queue_size += 1;

    auto &qdb = ui.quad_draw_buffer;

    const f32 x0 = Min(p0.x, p1.x);
    const f32 y0 = Min(p0.y, p1.y);

    const f32 x1 = Max(p0.x, p1.x);
    const f32 y1 = Max(p0.y, p1.y);
    
    vec2 *vp = qdb.positions + 4 * qdb.quad_count;
    u32  *vc = qdb.colors    + 4 * qdb.quad_count;

    vp[0] = vec2(x0, y0);
    vp[1] = vec2(x1, y0);
    vp[2] = vec2(x0, y1);    
    vp[3] = vec2(x1, y1);

    // @Cleanup: thats a bit unfortunate that we can't use instanced vertex advance rate
    // for color, maybe there is a clever solution somewhere.
    vc[0] = color;
    vc[1] = color;
    vc[2] = color;
    vc[3] = color;
    
    qdb.quad_count += 1;
}

void ui_flush() {
    PROFILE_SCOPE(__FUNCTION__);

    auto &tdb = ui.text_draw_buffer;
    auto &qdb = ui.quad_draw_buffer;

    s32 total_char_instance_offset = 0;
    s32 total_quad_instance_offset = 0;
    
    for (s32 i = 0; i < ui.draw_queue_size; ++i) {
        Render_Command r_cmd = {};
        r_cmd.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_BLEND | RENDER_FLAG_RESET;
        r_cmd.render_mode  = RENDER_TRIANGLE_STRIP;
        r_cmd.polygon_mode = POLYGON_FILL;
        r_cmd.viewport.x      = viewport.x;
        r_cmd.viewport.y      = viewport.y;
        r_cmd.viewport.width  = viewport.width;
        r_cmd.viewport.height = viewport.height;
        r_cmd.scissor.x      = viewport.x;
        r_cmd.scissor.y      = viewport.y;
        r_cmd.scissor.width  = viewport.width;
        r_cmd.scissor.height = viewport.height;
        r_cmd.cull_face.type    = CULL_FACE_BACK;
        r_cmd.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
        r_cmd.blend.source      = BLEND_SOURCE_ALPHA;
        r_cmd.blend.destination = BLEND_ONE_MINUS_SOURCE_ALPHA;
        r_cmd.instance_count = 0;
        r_cmd.buffer_element_count = 0;
    
        const s32 char_instance_offset = total_char_instance_offset;
        const s32 quad_instance_offset = total_quad_instance_offset;
        
        const auto &ui_draw_type = ui.draw_queue[i].type;
        const auto &atlas_index  = ui.draw_queue[i].atlas_index;
        
        // Detect similar adjacent ui commands and stack them in one render command.
        s32 last_adjacent_index = i;
        for (s32 j = i; j < ui.draw_queue_size; ++j) {
            const auto &ui_cmd = ui.draw_queue[j];
            const auto &atlas  = *ui.font_atlases[ui_cmd.atlas_index];

            // UI commands should have the same type and use the same atlas.
            if (ui_cmd.type != ui_draw_type || ui_cmd.atlas_index != atlas_index) {
                break;
            }
            
            last_adjacent_index = j;

            switch (ui_cmd.type) {
            case UI_DRAW_TEXT: {
                r_cmd.rid_vertex_array = tdb.rid_vertex_array;
                r_cmd.buffer_element_count = 4;
                r_cmd.instance_count += ui_cmd.element_count;
                r_cmd.instance_offset = char_instance_offset;

                r_cmd.sid_material = tdb.sid_material;
                r_cmd.rid_override_texture = atlas.rid_texture;

                total_char_instance_offset += ui_cmd.element_count;
            
                break;
            }
            case UI_DRAW_QUAD: {
                r_cmd.rid_vertex_array = qdb.rid_vertex_array;
                r_cmd.buffer_element_count += 4;
                r_cmd.buffer_element_offset = 4 * quad_instance_offset;

                // Instance count for quad should be 1 as for now vertex data for ui 
                // elements takes raw positions from vertex buffer unlike text that
                // takes transform matrix as vertex data and apply it to static quad
                // vertices.
                r_cmd.instance_count  = ui_cmd.element_count;
                r_cmd.instance_offset = quad_instance_offset;

                r_cmd.sid_material = qdb.sid_material;
            
                total_quad_instance_offset += ui_cmd.element_count;
            
                break;
            }
            }
        }
        
        i = last_adjacent_index;

        r_submit(&r_cmd);
    }

    ui.draw_queue_size = 0;
    tdb.char_count = 0;
    qdb.quad_count = 0;
}

void r_init_buffer_storages() {
    u32 storage_flags = R_FLAG_STORAGE_DYNAMIC | R_FLAG_MAP_WRITE | R_FLAG_MAP_PERSISTENT | R_FLAG_MAP_COHERENT;
    u32 map_flags = R_FLAG_MAP_WRITE | R_FLAG_MAP_PERSISTENT | R_FLAG_MAP_COHERENT;

    //#if DEVELOPER
    storage_flags |= R_FLAG_MAP_READ;
    map_flags     |= R_FLAG_MAP_READ;
    //#endif
    
    vertex_buffer_storage.rid = r_create_storage(null, MAX_VERTEX_STORAGE_SIZE, storage_flags);
    vertex_buffer_storage.size = 0;
    vertex_buffer_storage.capacity = MAX_VERTEX_STORAGE_SIZE;
    vertex_buffer_storage.mapped_data = r_map_buffer(vertex_buffer_storage.rid, 0, MAX_VERTEX_STORAGE_SIZE, map_flags);

    index_buffer_storage.rid = r_create_storage(null, MAX_INDEX_STORAGE_SIZE, storage_flags);
    index_buffer_storage.size = 0;
    index_buffer_storage.capacity = MAX_INDEX_STORAGE_SIZE;
    index_buffer_storage.mapped_data = r_map_buffer(index_buffer_storage.rid, 0, MAX_INDEX_STORAGE_SIZE, map_flags);

#if DEVELOPER
    EID_VERTEX_DATA_OFFSET = vertex_buffer_storage.size;
    EID_VERTEX_DATA_SIZE   = 0;
    EID_VERTEX_DATA = r_allocv(MAX_EID_VERTEX_DATA_SIZE);
#endif
}

void *r_allocv(u32 size) {
    auto &vbs = vertex_buffer_storage;
    
    void *data = (u8 *)vbs.mapped_data + vbs.size;
    vbs.size += size;
    Assert(vbs.size <= MAX_VERTEX_STORAGE_SIZE);
    
    return data;
}

void *r_alloci(u32 size) {
    auto &ibs = index_buffer_storage;
    
    void *data = (u8 *)ibs.mapped_data + ibs.size;
    ibs.size += size;
    Assert(ibs.size <= MAX_INDEX_STORAGE_SIZE);
    
    return data;
}

void cache_uniform_value_on_cpu(Uniform *uniform, const void *data, u32 data_size, u32 data_offset) {
    const u32 max_size = uniform->count * get_uniform_type_size(uniform->type);
    Assert(data_size + data_offset <= max_size);

    auto &cache = uniform_value_cache;
    Assert(cache.size + data_size + data_offset <= MAX_UNIFORM_VALUE_CACHE_SIZE);
    
    if (uniform->value_offset < MAX_UNIFORM_VALUE_CACHE_SIZE) {
        // Just update specified uniform value part if it was cached before.
        copy_bytes((u8 *)cache.data + uniform->value_offset + data_offset, data, data_size);
    } else {
        // Actually cache uniform value in global uniform value cache.
        uniform->value_offset = cache.size;
        copy_bytes((u8 *)cache.data + uniform->value_offset + data_offset, data, data_size);
        cache.size += data_size + data_offset;
    }
}

u32 get_uniform_type_size(Uniform_Type type) {
    switch (type) {
	case UNIFORM_U32:     return 1  * sizeof(u32);
	case UNIFORM_F32:     return 1  * sizeof(f32);
	case UNIFORM_F32_2:   return 2  * sizeof(f32);
	case UNIFORM_F32_3:   return 3  * sizeof(f32);
	case UNIFORM_F32_4:   return 4  * sizeof(f32);
	case UNIFORM_F32_4X4: return 16 * sizeof(f32);
    default:
        error("Failed to get uniform size from type %d", type);
        return 0;
    }
}

u32 get_uniform_type_dimension(Uniform_Type type) {
    switch (type) {
	case UNIFORM_U32:     return 1;
	case UNIFORM_F32:     return 1;
	case UNIFORM_F32_2:   return 2;
	case UNIFORM_F32_3:   return 3;
	case UNIFORM_F32_4:   return 4;
	case UNIFORM_F32_4X4: return 16;
    default:
        error("Failed to get uniform size from type %d", type);
        return 0;
    }
}

u32 get_uniform_block_field_size(const Uniform_Block_Field &field) {
    return get_uniform_type_size(field.type) * field.count;
}

const char *TYPE_NAME_U32  = "u32";
const char *TYPE_NAME_F32  = "f32";
const char *TYPE_NAME_VEC2 = "vec2";
const char *TYPE_NAME_VEC3 = "vec3";
const char *TYPE_NAME_VEC4 = "vec4";
const char *TYPE_NAME_MAT4 = "mat4";

void init_material_asset(Material *material, void *data) {
    // @Todo: parse material data - ideally text for editor and binary for release.
        
    enum Declaration_Type {
        DECL_NONE,
        DECL_SHADER,
        DECL_TEXTURE,
        DECL_AMBIENT,
        DECL_DIFFUSE,
        DECL_SPECULAR,
        DECL_UNIFORM,
    };

    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;

    const char *DECL_NAME_SHADER   = "s";
    const char *DECL_NAME_TEXTURE  = "t";
    const char *DECL_NAME_AMBIENT  = "la";
    const char *DECL_NAME_DIFFUSE  = "ld";
    const char *DECL_NAME_SPECULAR = "ls";
    const char *DECL_NAME_UNIFORM  = "u";

    const char *DELIMITERS = " ";

    material->uniform_count = 0;
    
    char *p = (char *)data;
    p[material->data_size] = '\0';

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_SHADER)) {
                decl_type = DECL_SHADER;
            } else if (str_cmp(token, DECL_NAME_TEXTURE)) {
                decl_type = DECL_TEXTURE;
            } else if (str_cmp(token, DECL_NAME_AMBIENT)) {
                decl_type = DECL_AMBIENT;
            } else if (str_cmp(token, DECL_NAME_DIFFUSE)) {
                decl_type = DECL_DIFFUSE;
            } else if (str_cmp(token, DECL_NAME_SPECULAR)) {
                decl_type = DECL_SPECULAR;
            } else if (str_cmp(token, DECL_NAME_UNIFORM)) {
                decl_type = DECL_UNIFORM;
            } else {
                error("Unknown material token declaration '%s'", token);
                continue;
            }

            switch (decl_type) {
            case DECL_SHADER: {
                char *sv = str_token(null, DELIMITERS);
                material->sid_shader = sid_intern(sv);
                break;
            }
            case DECL_TEXTURE: {
                char *sv = str_token(null, DELIMITERS);
                material->sid_texture = sid_intern(sv);
                break;
            }
            case DECL_AMBIENT: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->ambient = v;
                break;
            }
            case DECL_DIFFUSE: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->diffuse = v;
                break;
            }
            case DECL_SPECULAR: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->specular = v;
                break;
            }
            case DECL_UNIFORM: {
                Assert(material->uniform_count < MAX_MATERIAL_UNIFORMS);

                auto &uniform = material->uniforms[material->uniform_count];
                uniform.count = 1;

                char *su_name = str_token(null, DELIMITERS);
                str_copy(uniform.name, su_name);

                char *su_type = str_token(null, DELIMITERS);
                if (str_cmp(su_type, TYPE_NAME_U32)) {
                    uniform.type = UNIFORM_U32;

                    u32 v = 0;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_u32(sv);
                    }
                    
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_F32)) {
                    uniform.type = UNIFORM_F32;

                    f32 v = 0.0f;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_f32(sv);
                    }
                    
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC2)) {
                    uniform.type = UNIFORM_F32_2;

                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }
                     
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC3)) {
                    uniform.type = UNIFORM_F32_3;

                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC4)) {
                    uniform.type = UNIFORM_F32_4;

                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_MAT4)) {
                    uniform.type = UNIFORM_F32_4X4;

                    mat4 v = mat4_identity();
                    for (s32 i = 0; i < 16; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i % 4][i / 4] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else {
                    error("Unknown uniform type declaration '%s'", su_type);
                    continue;
                }

                material->uniform_count += 1;

                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

void set_material_uniform_value(Material *material, const char *uniform_name, const void *data, u32 size, u32 offset) {
    Uniform *uniform = null;
    for (s32 i = 0; i < material->uniform_count; ++i) {
        auto &u = material->uniforms[i];
        if (str_cmp(u.name, uniform_name)) {
            uniform = &u;
            break;
        }
	}

    if (!uniform) return;

    cache_uniform_value_on_cpu(uniform, data, size, offset);
}

void init_mesh_asset(Mesh *mesh, void *data) {
    // @Todo: parse material data - ideally text for editor and binary for release.
    
    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;
    
    enum Declaration_Type {
        DECL_NONE,
        DECL_VERTEX_COUNT,
        DECL_INDEX_COUNT,
        DECL_VERTEX_COMPONENT,
        DECL_DATA_GO,
        DECL_POSITION,
        DECL_NORMAL,
        DECL_INDEX,
        DECL_UV,
        
        DECL_COUNT
    };

    const char *DECL_NAME_VERTEX_COUNT      = "vn";
    const char *DECL_NAME_INDEX_COUNT       = "in";
    const char *DECL_NAME_VERTEX_COMPONENT  = "vc";
    const char *DECL_NAME_DATA_GO = "go";
    const char *DECL_NAME_POSITION = "p";
    const char *DECL_NAME_NORMAL   = "n";
    const char *DECL_NAME_UV       = "uv";
    const char *DECL_NAME_INDEX = "i";
    
    const char *DELIMITERS = " ";

    mesh->vertex_layout_size = 0;
    
    char *p = (char *)data;
    p[mesh->data_size] = '\0';

    u8 *r_vertex_data = (u8 *)vertex_buffer_storage.mapped_data;
    u8 *r_index_data  = (u8 *)index_buffer_storage.mapped_data;
    
    u32 vertex_offsets[MAX_MESH_VERTEX_COMPONENTS];

    s32 decl_to_vertex_layout_index[DECL_COUNT];
    Vertex_Component_Type decl_to_vct[DECL_COUNT];

    u32 index_data_offset = 0;

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_VERTEX_COUNT)) {
                decl_type = DECL_VERTEX_COUNT;
            } else if (str_cmp(token, DECL_NAME_INDEX_COUNT)) {
                decl_type = DECL_INDEX_COUNT;
            } else if (str_cmp(token, DECL_NAME_VERTEX_COMPONENT)) {
                decl_type = DECL_VERTEX_COMPONENT;
            } else if (str_cmp(token, DECL_NAME_DATA_GO)) {
                decl_type = DECL_DATA_GO;
            } else if (str_cmp(token, DECL_NAME_POSITION)) {
                decl_type = DECL_POSITION;
            } else if (str_cmp(token, DECL_NAME_NORMAL)) {
                decl_type = DECL_NORMAL;
            } else if (str_cmp(token, DECL_NAME_UV)) {
                decl_type = DECL_UV;
            } else if (str_cmp(token, DECL_NAME_INDEX)) {
                decl_type = DECL_INDEX;
            } else {
                error("Unknown mesh token declaration '%s'", token);
                continue;
            }
            
            switch (decl_type) {
            case DECL_VERTEX_COUNT: {
                char *sv = str_token(null, DELIMITERS);
                mesh->vertex_count = str_to_u32(sv);
                break;
            }
            case DECL_INDEX_COUNT: {
                char *sv = str_token(null, DELIMITERS);
                mesh->index_count = str_to_u32(sv);
                break;
            }
            case DECL_VERTEX_COMPONENT: {
                char *sdecl = str_token(null, DELIMITERS);
                char *stype = str_token(null, DELIMITERS);

                Vertex_Component_Type vct;
                if (str_cmp(stype, TYPE_NAME_VEC2)) {
                    vct = VERTEX_F32_2;
                } else if (str_cmp(stype, TYPE_NAME_VEC3)) {
                    vct = VERTEX_F32_3;                    
                } else if (str_cmp(stype, TYPE_NAME_VEC4)) {
                    vct = VERTEX_F32_4;
                } else {
                    error("Unknown mesh token vertex component type declaration '%s'", token);
                    continue;
                }

                Declaration_Type decl_vertex_type = DECL_NONE;
                if (str_cmp(sdecl, DECL_NAME_POSITION)) {
                    decl_vertex_type = DECL_POSITION;
                } else if (str_cmp(sdecl, DECL_NAME_NORMAL)) {
                    decl_vertex_type = DECL_NORMAL;
                } else if (str_cmp(sdecl, DECL_NAME_UV)) {
                    decl_vertex_type = DECL_UV;
                } else {
                    error("Unknown mesh token vertex component declaration '%s'", token);
                    continue;
                }

                decl_to_vct[decl_vertex_type] = vct;
                decl_to_vertex_layout_index[decl_vertex_type] = mesh->vertex_layout_size;

                Assert(mesh->vertex_layout_size < MAX_MESH_VERTEX_COMPONENTS);
                mesh->vertex_layout[mesh->vertex_layout_size] = vct;
                mesh->vertex_layout_size += 1;
                
                break;
            }
            case DECL_DATA_GO: {
                Assert(mesh->vertex_layout_size <= MAX_VERTEX_ARRAY_BINDINGS);

                mesh->vertex_size = 0;

                u32 offset = vertex_buffer_storage.size;
                Vertex_Array_Binding bindings[MAX_VERTEX_ARRAY_BINDINGS];
                s32 binding_count = 0;

                for (s32 i = 0; i < mesh->vertex_layout_size; ++i) {
                    const s32 vcs = get_vertex_component_size(mesh->vertex_layout[i]); 
                    mesh->vertex_size += vcs;

                    auto &binding = bindings[i];
                    binding.binding_index = i;
                    binding.data_offset = offset;
                    binding.layout_size = 1;
                    binding.layout[0] = { mesh->vertex_layout[i], 0 };
                    binding_count += 1;
                    
                    vertex_offsets[i] = offset;

                    const u32 size = vcs * mesh->vertex_count;
                    offset += size;
                }

                mesh->rid_vertex_array = r_create_vertex_array(bindings, binding_count);
                
                mesh->vertex_data_size   = mesh->vertex_size * mesh->vertex_count;
                mesh->vertex_data_offset = vertex_buffer_storage.size;

                vertex_buffer_storage.size += mesh->vertex_data_size;
                Assert(vertex_buffer_storage.size <= MAX_VERTEX_STORAGE_SIZE);

                mesh->index_data_size  = mesh->index_count  * sizeof(u32);
                mesh->index_data_offset = index_buffer_storage.size;

                index_buffer_storage.size += mesh->index_data_size;
                Assert(index_buffer_storage.size <= MAX_INDEX_STORAGE_SIZE);

                index_data_offset = mesh->index_data_offset;
                
                break;
            }
            case DECL_POSITION:
            case DECL_NORMAL:
            case DECL_UV: {
                const auto vct = decl_to_vct[decl_type];
                const s32 vli  = decl_to_vertex_layout_index[decl_type];
                
                if (decl_type == DECL_NORMAL) {
                    volatile int a = 0;
                }

                switch (vct) {
                case VERTEX_F32_2: {
                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case VERTEX_F32_3: {
                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case VERTEX_F32_4: {
                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                }
                
                break;
            }
            case DECL_INDEX: {
                char *sv = str_token(null, DELIMITERS);

                const u32 v = str_to_u32(sv);
                copy_bytes(r_index_data + index_data_offset, &v, sizeof(v));
                
                index_data_offset += sizeof(v);
                
                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

s32 get_vertex_component_dimension(Vertex_Component_Type type) {
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

s32 get_vertex_component_size(Vertex_Component_Type type) {
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

void update_render_stats() {
    static f32 update_time = 0.0f;
    constexpr f32 update_interval = 0.2f;

    update_time += delta_time;
    
    constexpr s32 dt_frame_count = 512;
    static f32 previous_dt_table[dt_frame_count];
    
    frame_index++;
    previous_dt_table[frame_index % dt_frame_count] = delta_time;

    if (update_time > update_interval) {
        update_time = 0.0f;
        
        f32 dt_sum = 0.0f;
        for (s32 i = 0; i < dt_frame_count; ++i) {
            dt_sum += previous_dt_table[i];
        }
        
        average_dt  = dt_sum / dt_frame_count;
        average_fps = 1.0f / average_dt;
    }
}

Texture_Format_Type get_texture_format_from_channel_count(s32 channel_count) {
    switch (channel_count) {
    case 3: return TEXTURE_FORMAT_RGB_8;
    case 4: return TEXTURE_FORMAT_RGBA_8;
    default:
        error("Not really handled case for texture channel count %d, using %d texture format", channel_count, TEXTURE_FORMAT_RGBA_8);
        return TEXTURE_FORMAT_RGBA_8;
    }
}

void init_texture_asset(Texture *texture, void *data) {
    if (texture->rid == RID_NONE) {
        texture->rid = r_create_texture(texture->type, texture->format, texture->width, texture->height, data);
    } else {
        r_set_texture_data(texture->rid, texture->type, texture->format, texture->width, texture->height, data);
    }
}

void set_texture_wrap(Texture *texture, Texture_Wrap_Type wrap) {
    texture->wrap = wrap;
    r_set_texture_wrap(texture->rid, wrap);
}

void set_texture_filter(Texture *texture, Texture_Filter_Type filter) {
    texture->filter = filter;

    const bool has_mipmaps = texture->flags & TEXTURE_FLAG_HAS_MIPMAPS;
    r_set_texture_filter(texture->rid, filter, has_mipmaps);
}

void generate_texture_mipmaps(Texture *texture) {
    texture->flags |= TEXTURE_FLAG_HAS_MIPMAPS;
    r_generate_texture_mipmaps(texture->rid);
}

void delete_texture(Texture *texture) {
    r_delete_texture(texture->rid);
    texture->rid = RID_NONE;
}

static For_Each_Result cb_draw_aabb(Entity *e, void *user_data) {
    auto *aabb = world->aabbs.find(e->aabb_index);
    if (aabb) {
        u32 aabb_color = rgba_black;
        switch (e->type) {
        case ENTITY_PLAYER:
        case ENTITY_STATIC_MESH: {
            aabb_color = rgba_red;
            break;
        }
        case ENTITY_SOUND_EMITTER_2D:
        case ENTITY_SOUND_EMITTER_3D: {
            aabb_color = rgba_blue;
            break;
        }
        case ENTITY_PORTAL: {
            aabb_color = rgba_purple;
            break;
        }
        case ENTITY_DIRECT_LIGHT:
        case ENTITY_POINT_LIGHT: {
            aabb_color = rgba_white;
            break;
        }
        }
        
        geo_draw_aabb(*aabb, aabb_color);
    }

    return RESULT_CONTINUE;
};

void geo_draw_debug() {
    const auto &player = world->player;

    if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
        const vec3 center = player.location + vec3(0.0f, player.scale.y * 0.5f, 0.0f);
        geo_draw_arrow(center, center + normalize(player.velocity) * 0.5f, rgba_red);
    }
    
    if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
        for_each_entity(world, cb_draw_aabb);

        if (world->mouse_picked_entity) {
            const auto *e = world->mouse_picked_entity;
            const u32 mouse_picked_color = rgba_yellow;
            geo_draw_aabb(world->aabbs[e->aabb_index], mouse_picked_color);
        }
        
        if (player.collide_aabb_index != INVALID_INDEX) {
            geo_draw_aabb(world->aabbs[player.aabb_index],         rgba_green);
            geo_draw_aabb(world->aabbs[player.collide_aabb_index], rgba_green);
        }
    }
}
