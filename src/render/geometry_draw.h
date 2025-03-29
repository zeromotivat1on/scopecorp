#pragma once

inline constexpr s32 MAX_GEOMETRY_LINE_COUNT   = 1024;
inline constexpr s32 MAX_GEOMETRY_VERTEX_COUNT = 2 * MAX_GEOMETRY_LINE_COUNT;

struct vec3;
struct AABB;

struct Geometry_Draw_Buffer {
    f32 *vertex_data = null;
    s32 vertex_count = 0;
    s32 vertex_array_index = INVALID_INDEX;
    s32 material_index     = INVALID_INDEX;
};

inline Geometry_Draw_Buffer geometry_draw_buffer;

void init_geo_draw();
void draw_geo_line(vec3 start, vec3 end, vec3 color);
void draw_geo_arrow(vec3 start, vec3 end, vec3 color);
void draw_geo_box(const vec3 points[8], vec3 color);
void draw_geo_aabb(const AABB &aabb, vec3 color);
void flush_geo_draw();

void draw_geo_debug();
