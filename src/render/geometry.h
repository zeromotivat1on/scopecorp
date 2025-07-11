#pragma once

inline constexpr u32 MAX_GEOMETRY_LINE_COUNT   = 1024;
inline constexpr u32 MAX_GEOMETRY_VERTEX_COUNT = 2 * MAX_GEOMETRY_LINE_COUNT;

struct vec3;
struct AABB;
struct Ray;

struct Geometry_Draw_Buffer {
    vec3 *locations = null;
    u32  *colors    = null;
    
    u32 vertex_count = 0;

    rid rid_vertex_array = RID_NONE;
    sid sid_material = SID_NONE;
};

inline Geometry_Draw_Buffer geo_draw_buffer;

void geo_init();
void geo_draw_line(vec3 start, vec3 end, u32 color);
void geo_draw_arrow(vec3 start, vec3 end, u32 color);
void geo_draw_cross(vec3 location, f32 size);
void geo_draw_box(const vec3 points[8], u32 color);
void geo_draw_aabb(const AABB &aabb, u32 color);
void geo_draw_ray(const Ray &ray, f32 length, u32 color);
void geo_flush();

void draw_geo_debug();
