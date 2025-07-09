#begin vertex
#version 460 core

#include "viewport.glsl.h"

layout (location = 0) in vec2 v_position;
layout (location = 1) in uint v_color;

layout (location = 0) out uint f_color;

void main() {
    gl_Position = u_viewport_ortho * vec4(v_position, 0.0f, 1.0f);
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
