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

AABB resolve(const AABB &moving, const AABB &stable) {
    assert(overlap(moving, stable)); // intentionally in assert, as it should be done before
    
    AABB resolved = moving;
    
    const vec3 moving_size = abs(moving.max - moving.min);
    const vec3 min_delta   = abs(moving.min - stable.min);
    const vec3 max_delta   = abs(moving.max - stable.max);
    
    if (min_delta.x < moving_size.x) {
        const f32 delta = moving_size.x - min_delta.x;
        resolved.min.x -= delta;
    }
    if (max_delta.x < moving_size.x) {
        const f32 delta = moving_size.x - max_delta.x;
        resolved.max.x += delta;
    }

    if (min_delta.y < moving_size.y) {
        const f32 delta = moving_size.y - min_delta.y;
        resolved.min.y -= delta;
    }
    if (max_delta.y < moving_size.y) {
        const f32 delta = moving_size.y - max_delta.y;
        resolved.max.y += delta;
    }

    if (min_delta.z < moving_size.z) {
        const f32 delta = moving_size.z - min_delta.z;
        resolved.min.z -= delta;
    }
    if (max_delta.z < moving_size.z) {
        const f32 delta = moving_size.z - max_delta.z;
        resolved.max.z += delta;
    }
    
    return resolved;
}
