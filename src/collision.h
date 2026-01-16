#pragma once

struct Camera;
struct Viewport;
struct Entity;

struct Collision_Result {
    bool    hit  = false;
    f32     time = 0.0f;
    Vector3 direction;
};

struct AABB {
	Vector3 c;
	Vector3 r;
};

struct Sphere {
    Vector3 c;
    f32     r;
};

struct Ray {
    Vector3 origin;
    Vector3 direction;
};

bool overlap (AABB a, AABB b);
bool overlap (Sphere a, Sphere b);
bool overlap (Sphere s, AABB b);
bool overlap (Ray r, AABB b, f32 *n = null);
bool inside  (Vector3 p, AABB b);
bool inside  (Vector3 p, Sphere s);
bool inside  (Vector2 p, Vector2 p0, Vector2 p1);

Ray ray_from_mouse (const Camera &camera, const Viewport &viewport, s16 x, s16 y);

Direction get_direction     (Vector3 t);
Vector3   get_closest_point (Vector3 p, AABB b);
f32       distance_sqr      (Vector3 p, AABB b);

inline AABB make_aabb (Vector3 c, Vector3 r)                           { return AABB { c, r }; };
inline AABB make_aabb (f32 cx, f32 cy, f32 cz, f32 rx, f32 ry, f32 rz) { return AABB { Vector3(cx, cy, cz), Vector3(rx, ry, rz) }; };

//template <typename T> f32 get_max_movement_over_time (T *a, f32 t0, f32 t1);
//template <typename T> f32 get_min_distance_at_time   (T *a, T *b, f32 t);

template <typename T>
auto interval_collision (T *a, T *b, f32 t0, f32 t1) -> Collision_Result {
    static const auto do_collision = [] (T *a, T *b, f32 t0, f32 t1, f32 &t) -> bool {
        constexpr auto INTERVAL_EPSILON = 0.001f;
            
        const auto max_move_a    = get_max_movement_over_time(a, t0, t1);
        const auto max_move_b    = get_max_movement_over_time(b, t0, t1);
        const auto max_move_dist = max_move_a + max_move_b;

        const auto min_dist_start = get_min_distance_at_time(a, b, t0);
        if (min_dist_start > max_move_dist) return false;

        const auto min_dist_end = get_min_distance_at_time(a, b, t1);
        if (min_dist_end > max_move_dist) return false;

        if (t1 - t0 < INTERVAL_EPSILON) {
            t = t0;
            return true;
        }

        const auto mid = (t0 + t1) * 0.5f;
        if (do_collision(a, b, t0, mid, t)) return true;

        return do_collision(a, b, mid, t1, t);
    };

    Collision_Result cr;
    cr.hit       = do_collision(a, b, t0, t1, cr.time);
    cr.direction = Vector3_zero;
    
    return cr;
}
