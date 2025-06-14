#begin vertex
#version 460 core

layout (location = 0) in vec3 v_location;;

uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * vec4(v_location, 1.0f);
}
#end vertex

#begin fragment
#version 460 core

#include "color.glsl.h"

layout (location = 0) out vec4 out_color;

uniform uint u_color;

void main() {
    out_color = rgba_unpack(u_color);
}
#end fragment
