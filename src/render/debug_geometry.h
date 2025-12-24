#pragma once

#include "command_buffer.h"
#include "vertex_descriptor.h"

struct Ray;
struct Material;

struct Line_Geometry {
    static constexpr u32 MAX_LINES    = 1024;
    static constexpr u32 MAX_VERTICES = 2 * MAX_LINES;
    
    Vector3 *positions = null;
    u32  *colors    = null;
    
    u32 vertex_count = 0;

    Vertex_Descriptor vertex_descriptor;
};

inline Line_Geometry line_geometry;

void init_line_geometry ();

void draw_line  (Vector3 start, Vector3 end, u32 color);
void draw_arrow (Vector3 start, Vector3 end, u32 color);
void draw_cross (Vector3 position, f32 size);
void draw_box   (const Vector3 points[8], u32 color);
void draw_aabb  (const AABB &aabb, u32 color);
void draw_ray   (const Ray &ray, f32 length, u32 color);
