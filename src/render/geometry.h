#pragma once

inline constexpr s32 MAX_GEOMETRY_LINE_COUNT   = 1024;
inline constexpr s32 MAX_GEOMETRY_VERTEX_COUNT = 2 * MAX_GEOMETRY_LINE_COUNT;

struct vec3;
struct AABB;

struct Geometry_Draw_Buffer {
    rid rid_vertex_array = RID_NONE;
    sid sid_material = SID_NONE;

    f32 *vertex_data = null;
    u32 vertex_count = 0;    
};

inline Geometry_Draw_Buffer geo_draw_buffer;

void geo_init();
void geo_draw_line(vec3 start, vec3 end, vec3 color);
void geo_draw_arrow(vec3 start, vec3 end, vec3 color);
void geo_draw_cross(vec3 location, f32 size);
void geo_draw_box(const vec3 points[8], vec3 color);
void geo_draw_aabb(const AABB &aabb, vec3 color);
void geo_flush();

void geo_draw_debug();
