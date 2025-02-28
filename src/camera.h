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
};

mat4 camera_view(const Camera* camera);
mat4 camera_projection(const Camera* camera);
void on_viewport_resize(Camera* camera, struct Viewport* viewport);
