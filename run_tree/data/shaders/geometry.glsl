#begin vertex
#version 460 core

#include "camera.glsl.h"

layout (location = 0) in vec3 v_location;
layout (location = 1) in uint v_color;

layout (location = 0) out uint f_color;

void main() {
    gl_Position = u_camera_view_proj * vec4(v_location, 1.0f);
    f_color = v_color;
}
#end vertex

#begin fragment
#version 460 core

#include "color.glsl.h"

layout (location = 0) in flat uint f_color;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = rgba_unpack(f_color);
}
#end fragment