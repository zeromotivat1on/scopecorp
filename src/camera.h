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
};

struct Viewport;

void update_matrices(Camera *camera);
void on_viewport_resize(Camera *camera, const Viewport *viewport);
