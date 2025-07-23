#pragma once

#include "render/r_command.h"

struct vec3;
struct AABB;
struct Ray;

struct R_Geo {
    static constexpr u32 MAX_COMMANDS = 1;
    static constexpr u32 MAX_LINES    = 1024;
    static constexpr u32 MAX_VERTICES = 2 * MAX_LINES;
    
    vec3 *locations = null;
    u32  *colors    = null;
    
    u32 vertex_count = 0;

    u16 vertex_descriptor = RID_NONE;
    sid sid_material = SID_NONE;

    R_Command_List command_list;
};

inline R_Geo R_geo;

void r_geo_init();
void r_geo_line(vec3 start, vec3 end, u32 color);
void r_geo_arrow(vec3 start, vec3 end, u32 color);
void r_geo_cross(vec3 location, f32 size);
void r_geo_box(const vec3 points[8], u32 color);
void r_geo_aabb(const AABB &aabb, u32 color);
void r_geo_ray(const Ray &ray, f32 length, u32 color);
void r_geo_flush();

void r_geo_debug();
