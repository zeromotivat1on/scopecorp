#begin vertex
#version 460 core

#include "uniform_blocks.glsl.h"

layout (location = 0) in vec2 v_position;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 f_color;

void main() {
    gl_Position = u_viewport_ortho * vec4(v_position, 0.0f, 1.0f);
    f_color = v_color;
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec4 f_color;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = f_color;
}
#end fragment
