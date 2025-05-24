#include "pch.h"
#include "collision.h"
#include "profile.h"
#include "camera.h"

#include "math/math_core.h"

#include "render/viewport.h"

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
    const f32 distance_sqr = (point - sphere.pos).length_sqr();
    return distance_sqr < sphere.radius * sphere.radius;
}

bool overlap(const Sphere &a, const Sphere &b) {
    const f32 distance_sqr = (a.pos - b.pos).length_sqr();
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

s32 find_closest_overlapped_aabb(const Ray &ray, AABB *aabbs, s32 count) {
    PROFILE_SCOPE(__FUNCTION__);
    
    s32 closest_aabb_index = INVALID_INDEX;
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

vec3 viewport_to_world_direction(const Camera *camera, const Viewport *viewport, s16 x, s16 y) {
    vec3 ray_nds;
    ray_nds.x = (2.0f * (x - viewport->x)) / viewport->width - 1.0f;
    ray_nds.y = 1.0f - (2.0f * (y - viewport->y)) / viewport->height;
    ray_nds.z = 1.0f;

    const vec4 ray_clip = vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f);
    vec4 ray_eye = inverse(camera->proj) * ray_clip;
    ray_eye.z = -1.0f;
    ray_eye.w =  0.0f;

    const vec4 ray_world = inverse(camera->view) * ray_eye;
    return normalize(ray_world.to_vec3());
}

Ray world_ray_from_viewport_location(const Camera *camera, const Viewport *viewport, s16 x, s16 y) {
    Ray ray;
    ray.origin = camera->eye;
    ray.direction = viewport_to_world_direction(camera, viewport, x, y);
    return ray;
}
