#include "pch.h"
#include "collision.h"
#include "assertion.h"
#include "math/math_core.h"

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
    const f32 x = clamp(sphere.pos.x, aabb.min.x, aabb.max.x);
    const f32 y = clamp(sphere.pos.y, aabb.min.y, aabb.max.y);
    const f32 z = clamp(sphere.pos.z, aabb.min.z, aabb.max.z);
    return overlap(vec3(x, y, z), sphere);
}

AABB resolve_moving_stable(const AABB &a, const AABB &b, const vec3 &velocity_a) {
    const AABB moved_a = AABB{a.min + velocity_a, a.max + velocity_a};
    assert(overlap(moved_a, b)); // intentionally in assert, as it should be done before

    AABB resolved;
    
    
    return resolved;
}
