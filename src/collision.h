#pragma once

#include "math/vector.h"
#include "math/quat.h"

struct AABB {
	vec3 min;
	vec3 max;
};

bool aabb_overlap(const AABB &a, const AABB &b);
