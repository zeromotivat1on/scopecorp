#pragma once

#include "math/vector.h"
#include "math/matrix.h"

enum Camera_Mode {
	MODE_PERSPECTIVE,
	MODE_ORTHOGRAPHIC
};

struct Camera {
	Camera_Mode mode;

	vec3 at; // view point location
	vec3 up; // vertical direction
	vec3 eye; // camera location

	f32 yaw; // horizontal rotation around y-axis
	f32 pitch; // vertical rotation around x-axis

	f32	fov;
	f32	aspect;

	f32	near;
	f32	far;
	f32	left;
	f32	right;
	f32	bottom;
	f32	top;

    mat4 view;
    mat4 proj;
    mat4 view_proj;

    mat4 inv_view;
    mat4 inv_proj;
};

struct R_Viewport;

void update_matrices(Camera &c);
void on_viewport_resize(Camera &c, const R_Viewport &vp);

vec2 world_to_screen(vec3 loc, const Camera &camera, const Rect &rect);
vec3 screen_to_world(vec2 pos, const Camera &camera, const Rect &rect);
