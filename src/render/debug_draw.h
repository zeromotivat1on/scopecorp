#pragma once

#include "math/vector.h"

inline constexpr s32 MAX_DEBUG_DRAW_LINE_COUNT = 1024;

struct Debug_Line {
    vec3 start;
    vec3 end;
    vec3 color;
};

struct Debug_Geometry_Draw_Queue {
    f32 *vertex_data = null;
    Debug_Line *lines = null;
    s32 line_count = 0;
};

inline Debug_Geometry_Draw_Queue debug_draw_queue;

struct AABB;

void init_debug_geometry_draw_queue();
void draw_debug_line(vec3 start, vec3 end, vec3 color);
void draw_debug_box(vec3 points[8], vec3 color);
void draw_debug_aabb(const AABB &aabb, vec3 color);

void flush(Debug_Geometry_Draw_Queue* queue);
