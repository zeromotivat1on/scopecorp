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

s32 find_closest_overlapped_aabb(const Ray &ray, const World *world);

vec3 resolve_moving_static(const AABB &a, const AABB &b, const vec3 &velocity_a);

vec3 ray_from_mouse_position(const Camera *camera, const Viewport *viewport, s16 mouse_x, s16 mouse_y);

