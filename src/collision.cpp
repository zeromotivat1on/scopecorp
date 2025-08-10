#include "pch.h"
#include "collision.h"
#include "profile.h"
#include "camera.h"

#include "math/math_basic.h"

#include "render/r_viewport.h"

#include "game/world.h"

bool overlap(const AABB &a, const AABB &b) {
	return a.max.x >= b.min.x && a.min.x <= b.max.x
		&& a.max.y >= b.min.y && a.min.y <= b.max.y
		&& a.max.z >= b.min.z && a.min.z <= b.max.z;
}

bool overlap(const vec3 &point, const AABB &aabb) {
    return point.x >= aabb.min.x && point.x <= aabb.max.x
        && point.y >= aabb.min.y && point.y <= aabb.max.y
        && point.z >= aabb.min.z && point.z <= aabb.max.z;
}

bool overlap(const vec3 &point, const Sphere &sphere) {
    const f32 distance_sqr = length_sqr(point - sphere.pos);
    return distance_sqr < sphere.radius * sphere.radius;
}

bool overlap(const Sphere &a, const Sphere &b) {
    const f32 distance_sqr = length_sqr(a.pos - b.pos);
    return distance_sqr < a.radius * a.radius + b.radius * b.radius;
}

bool overlap(const Sphere &sphere, const AABB &aabb) {
    const f32 x = Clamp(sphere.pos.x, aabb.min.x, aabb.max.x);
    const f32 y = Clamp(sphere.pos.y, aabb.min.y, aabb.max.y);
    const f32 z = Clamp(sphere.pos.z, aabb.min.z, aabb.max.z);
    return overlap(vec3(x, y, z), sphere);
}

bool overlap(const Ray &ray, const AABB &aabb, f32 *near) {
    f32 tmin = 0.0f;
    f32 tmax = F32_MAX;

    vec3 ray_dir_inv = vec3(1);
    ray_dir_inv /= ray.direction;
    
    for (s32 d = 0; d < 3; ++d) {
        const f32 t1 = (aabb.min[d] - ray.origin[d]) * ray_dir_inv[d];
        const f32 t2 = (aabb.max[d] - ray.origin[d]) * ray_dir_inv[d];

        tmin = Min(Max(t1, tmin), Max(t2, tmin));
        tmax = Max(Min(t1, tmax), Min(t2, tmax));
    }

    if (near) *near = tmin;
    
    return tmin <= tmax;
}

bool inside(const vec2 &point, const vec2 &p0, const vec2 &p1) {
    const f32 x0 = Min(p0.x, p1.x);
    const f32 y0 = Min(p0.y, p1.y);

    const f32 x1 = Max(p0.x, p1.x);
    const f32 y1 = Max(p0.y, p1.y);
    
    return point.x >= x0 && point.x <= x1 && point.y >= y0 && point.y <= y1;
}

s32 find_closest_overlapped_aabb(const Ray &ray, AABB *aabbs, s32 count) {    
    s32 closest_aabb_index = INDEX_NONE;
    f32 min_distance = F32_MAX;
    for (s32 i = 0; i < count; ++i) {
        f32 near = 0.0f;
        if (!overlap(ray, aabbs[i], &near)) continue;
        
        if (near < min_distance) {
            min_distance = near;
            closest_aabb_index = i;
        }
    }

    return closest_aabb_index;
}

vec3 resolve_moving_static(const AABB &a, const AABB &b, const vec3 &velocity_a) {
    const AABB moved_a = AABB{a.min + velocity_a, a.max + velocity_a};
    if (!overlap(moved_a, b)) return velocity_a;
    
    vec3 resolved_velocity = vec3_zero;
    // @Cleanup: return zero vector is generally fine for small velocities
    // @Todo: resolve collision properly later
    
    return resolved_velocity;
}

Rect to_rect(const R_Viewport &vp) {
    return Rect { (f32)vp.x, (f32)vp.y, (f32)vp.width, (f32)vp.height };
}

Ray ray_from_mouse(const Camera &camera, const R_Viewport &viewport, s16 x, s16 y) {
    Ray ray;
    ray.origin = camera.eye;
    ray.direction = normalize(screen_to_world(to_rect(viewport),
                                              inverse(camera.view),
                                              inverse(camera.proj),
                                              vec2(x, y)));
    return ray;
}
