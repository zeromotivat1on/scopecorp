#include "pch.h"
#include "render/debug_draw.h"
#include "render/draw.h"
#include "render/vertex_buffer.h"

#include "memory_storage.h"

static s32 vertex_buffer_index = INVALID_INDEX;

void init_debug_geometry_draw_queue() {
    debug_draw_queue.lines = push_array(pers, MAX_DEBUG_DRAW_QUEUE_LINE_COUNT, Debug_Line);

    constexpr s32 vertex_count = 0;
    Vertex_Attrib_Type attribs[] = {VERTEX_ATTRIB_F32_V3};
    vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), null, vertex_count, BUFFER_USAGE_STREAM);
}

void draw_debug_line(vec3 start, vec3 end, vec3 color) {
    
}

void draw_debug_box(vec3 points[8], vec3 color) {
    
}

void flush_debug_geometry_draw_commands() {
    Draw_Command command;
    for (s32 i = 0; i < debug_draw_queue.line_count; ++i) {
        
    }
}

