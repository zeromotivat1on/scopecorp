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
    
    Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V3 };
    debug_geometry_vbi = create_vertex_buffer(attribs, c_array_count(attribs), null, MAX_DEBUG_DRAW_VERTEX_COUNT, BUFFER_USAGE_STREAM);

    const Uniform u_mvp = Uniform("u_mvp", UNIFORM_F32_MAT4, 1);
    debug_geometry_mti = render_registry.materials.add(Material(shader_index_list.debug_geometry, INVALID_INDEX));
    add_material_uniforms(debug_geometry_mti, &u_mvp);
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
    const mat4 mvp = mat4_identity() * view * proj;
    set_material_uniform_value(debug_geometry_mti, "u_mvp", mvp.ptr());
    
    set_vertex_buffer_data(debug_geometry_vbi, queue->vertex_data, 6 * 2 * queue->line_count * sizeof(f32));

    Draw_Command command;
    command.draw_mode = DRAW_LINES;
    command.vertex_buffer_index = debug_geometry_vbi;
    command.material_index = debug_geometry_mti;
    command.draw_count = 2 * queue->line_count;
    draw(&command);
    
    queue->line_count = 0;
}

