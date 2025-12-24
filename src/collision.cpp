#include "pch.h"
#include "collision.h"
#include "profile.h"
#include "viewport.h"
#include "world.h"

bool overlap(AABB a, AABB b) {
    if (Abs(a.c.x - b.c.x) >= (a.r.x + b.r.x)) return false;
    if (Abs(a.c.y - b.c.y) >= (a.r.y + b.r.y)) return false;
    if (Abs(a.c.z - b.c.z) >= (a.r.z + b.r.z)) return false;
    return true;
}

bool overlap(Sphere a, Sphere b) {
    const auto d = length_sqr(a.c - b.c);
    const auto r = a.r + b.r;
    return d < Sqr(r);
}

bool overlap(Sphere s, AABB b) {
    const auto d = distance_sqr(s.c, b);
    return d < Sqr(s.r);
}

bool inside(Vector3 p, AABB b) {
    return Abs(p.x - b.c.x) < b.r.x
        && Abs(p.y - b.c.y) < b.r.y
        && Abs(p.z - b.c.z) < b.r.z;
}

bool inside(Vector3 p, Sphere s) {
    const auto d = length_sqr(p - s.c);
    return d < Sqr(s.r);
}

bool inside(Vector2 point, Vector2 p0, Vector2 p1) {
    const auto x0 = Min(p0.x, p1.x);
    const auto y0 = Min(p0.y, p1.y);

    const auto x1 = Max(p0.x, p1.x);
    const auto y1 = Max(p0.y, p1.y);
    
    return point.x > x0 && point.x < x1 && point.y > y0 && point.y < y1;
}

// bool overlap(Ray ray, AABB aabb, f32 *near_distance) {
//     f32 tmin = 0.0f;
//     f32 tmax = F32_MAX;

//     Vector3 ray_dir_inv = Vector3(1);
//     ray_dir_inv /= ray.direction;
    
//     for (s32 d = 0; d < 3; ++d) {
//         const f32 t1 = (aabb.min[d] - ray.origin[d]) * ray_dir_inv[d];
//         const f32 t2 = (aabb.max[d] - ray.origin[d]) * ray_dir_inv[d];

//         tmin = Min(Max(t1, tmin), Max(t2, tmin));
//         tmax = Max(Min(t1, tmax), Min(t2, tmax));
//     }

//     if (near_distance) *near_distance = tmin;
    
//     return tmin <= tmax;
// }

Rect make_rect(const Viewport &vp) {
    return Rect { (f32)vp.x, (f32)vp.y, (f32)vp.width, (f32)vp.height };
}

Ray ray_from_mouse(const Camera &camera, const Viewport &viewport, s16 x, s16 y) {
    Ray ray;
    ray.origin = camera.position;
    ray.direction = normalize(screen_to_world(Vector2(x, y), camera, make_rect(viewport)));
    return ray;
}

auto get_direction(Vector3 t) -> Direction {
    const Vector3 compass[DIRECTION_COUNT] = {
        {  0, -1,  0 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
    };

    auto max = 0.0f;
    auto best_match = 0;
    for (auto i = 0; i < DIRECTION_COUNT; i++) {
        const auto dp = dot(normalize(t), compass[i]);
        if (dp > max) {
            max = dp;
            best_match = i;
        }
    }
    
    return (Direction)best_match;
}

auto get_closest_point(Vector3 p, AABB b) -> Vector3 {
    auto q = Vector3_zero;

    for (auto i = 0; i < 3; ++i) {
        const auto min = b.c[i] - b.r[i];
        const auto max = b.c[i] + b.r[i];

        auto v = p[i];
        v = Max(v, min);
        v = Min(v, max);

        q[i] = v;
    }
    
    return q;
}

auto distance_sqr(Vector3 p, AABB b) -> f32 {
    auto d = 0.0f;

    for (auto i = 0; i < 3; ++i) {
        const auto v   = p[i];
        const auto min = b.c[i] - b.r[i];
        const auto max = b.c[i] + b.r[i];

        if (v < min) d += Sqr(min - v);
        if (v > max) d += Sqr(v - max);
    }
    
    return d;
}
