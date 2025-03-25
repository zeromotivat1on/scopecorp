#include "pch.h"
#include "render/debug_draw.h"
#include "render/draw.h"
#include "render/viewport.h"
#include "render/render_registry.h"

#include "game/world.h"

#include "memory_storage.h"

static constexpr s32 MAX_DEBUG_DRAW_VERTEX_COUNT = 2 * MAX_DEBUG_DRAW_LINE_COUNT;
static constexpr s32 MAX_DEBUG_DRAW_BUFFER_SIZE  = 6 * MAX_DEBUG_DRAW_VERTEX_COUNT;

static s32 debug_geometry_vbi = INVALID_INDEX;
static s32 debug_geometry_mti = INVALID_INDEX;

void init_debug_geometry_draw_queue() {
    debug_draw_queue.lines = push_array(pers, MAX_DEBUG_DRAW_LINE_COUNT, Debug_Line);
    debug_draw_queue.vertex_data = push_array(pers, MAX_DEBUG_DRAW_BUFFER_SIZE, f32);
    
    Vertex_Component_Type components[] = { VERTEX_F32_3, VERTEX_F32_3 };
    debug_geometry_vbi = create_vertex_buffer(components, c_array_count(components), null, MAX_DEBUG_DRAW_VERTEX_COUNT, BUFFER_USAGE_STREAM);

    const s32 u_mvp = create_uniform("u_transform", UNIFORM_F32_4X4, 1);
    debug_geometry_mti = create_material(shader_index_list.debug_geometry, INVALID_INDEX);
    set_material_uniforms(debug_geometry_mti, &u_mvp, 1);
}

void draw_debug_line(vec3 start, vec3 end, vec3 color) {
    assert(debug_draw_queue.line_count < MAX_DEBUG_DRAW_LINE_COUNT);
    debug_draw_queue.lines[debug_draw_queue.line_count] = Debug_Line{start, end, color};
    
    f32 *v = debug_draw_queue.vertex_data + 6 * 2 * debug_draw_queue.line_count;

    v[0] = start[0];
    v[1] = start[1];
    v[2] = start[2];
    v[3] = color[0];
    v[4] = color[1];
    v[5] = color[2];

    v[6]  = end[0];
    v[7]  = end[1];
    v[8]  = end[2];
    v[9]  = color[0];
    v[10] = color[1];
    v[11] = color[2];

    debug_draw_queue.line_count++;
}

void draw_debug_box(vec3 points[8], vec3 color) {
    for (int i = 0; i < 4; ++i) {
        draw_debug_line(points[i],     points[(i + 1) % 4],       color);
        draw_debug_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        draw_debug_line(points[i],     points[i + 4],             color);
    }
}

void draw_debug_aabb(const AABB &aabb, vec3 color) {
    vec3 bb[2] = { aabb.min, aabb.max };
    vec3 points[8];
    
    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    draw_debug_box(points, color);
}

void flush(Debug_Geometry_Draw_Queue* queue) {
    if (queue->line_count == 0) return;
    
    Camera *camera = desired_camera(world);
	const mat4 view = camera_view(camera);
	const mat4 proj = camera_projection(camera);
    const mat4 vp = view * proj;
    set_material_uniform_value(debug_geometry_mti, "u_transform", vp.ptr());
    
    set_vertex_buffer_data(debug_geometry_vbi, queue->vertex_data, 6 * 2 * queue->line_count * sizeof(f32));

    Draw_Command command{};
    command.flags = DRAW_FLAG_SCISSOR_TEST | DRAW_FLAG_CULL_FACE_TEST | DRAW_FLAG_DEPTH_TEST | DRAW_FLAG_RESET;
    command.draw_mode    = DRAW_LINES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor_test.x      = viewport.x;
    command.scissor_test.y      = viewport.y;
    command.scissor_test.width  = viewport.width;
    command.scissor_test.height = viewport.height;
    command.cull_face_test.type    = CULL_FACE_BACK;
    command.cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command.depth_test.function = DEPTH_TEST_LESS;
    command.depth_test.mask     = DEPTH_TEST_ENABLE;
    command.vertex_buffer_index = debug_geometry_vbi;

    const auto &material  = render_registry.materials[debug_geometry_mti];
    command.shader_index  = material.shader_index;
    command.uniform_count = material.uniform_count;

    for (s32 i = 0; i < material.uniform_count; ++i) {
        command.uniform_indices[i]       = material.uniform_indices[i];
        command.uniform_value_offsets[i] = material.uniform_value_offsets[i];
    }

    command.buffer_element_count = 2 * queue->line_count;
    
    draw(&command);
    
    queue->line_count = 0;
}

