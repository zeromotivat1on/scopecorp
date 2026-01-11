#pragma once

struct Ray;

struct Line_Geometry {
    static constexpr u32 MAX_LINES    = 1024;
    static constexpr u32 MAX_VERTICES = 2 * MAX_LINES;
    
    Vector3 *positions;
    Color32 *colors;
    u64      positions_offset;
    u64      colors_offset;
    u32      vertex_input;
    u32      vertex_count;
};

void init_line_geometry ();
void draw_line          (Vector3 start, Vector3 end, Color32 color);
void draw_arrow         (Vector3 start, Vector3 end, Color32 color);
void draw_cross         (Vector3 position, f32 size);
void draw_box           (const Vector3 points[8], Color32 color);
void draw_aabb          (const AABB &aabb, Color32 color);
void draw_ray           (const Ray &ray, f32 length, Color32 color);

inline Line_Geometry line_geometry;
