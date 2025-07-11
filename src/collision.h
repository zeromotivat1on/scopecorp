#pragma once

#include "math/vector.h"
#include "math/quat.h"

struct World;
struct Camera;
struct Viewport;

struct AABB {
	vec3 min;
	vec3 max;
};

struct Sphere {
    vec3 pos;
    f32  radius;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

bool overlap(const AABB &a, const AABB &b);
bool overlap(const vec3 &point, const AABB &aabb);
bool overlap(const vec3 &point, const Sphere &sphere);
bool overlap(const Sphere &a, const Sphere &b);
bool overlap(const Sphere &sphere, const AABB &aabb);
bool overlap(const Ray &ray, const AABB &aabb, f32 *near = null);
bool inside(const vec2 &point, const vec2 &p0, const vec2 &p1);

s32 find_closest_overlapped_aabb(const Ray &ray, AABB *aabbs, s32 count);

vec3 resolve_moving_static(const AABB &a, const AABB &b, const vec3 &velocity_a);

vec3 direction_from_mouse(const Camera *camera, const Viewport *viewport, s16 x, s16 y);
Ray ray_from_mouse(const Camera *camera, const Viewport *viewport, s16 x, s16 y);
