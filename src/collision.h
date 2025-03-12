#pragma once

#include "math/vector.h"
#include "math/quat.h"

struct AABB {
	vec3 min;
	vec3 max;
};

struct Sphere {
    vec3 pos;
    f32  radius;
};

bool overlap(const AABB &a, const AABB &b);
bool overlap(const vec3 &point, const AABB &aabb);
bool overlap(const vec3 &point, const Sphere &sphere);
bool overlap(const Sphere &a, const Sphere &b);
bool overlap(const Sphere &sphere, const AABB &aabb);

vec3 resolve_moving_static(const AABB &a, const AABB &b, const vec3 &velocity_a);
