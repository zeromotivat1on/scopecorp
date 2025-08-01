#begin vertex
#version 460 core

#include "viewport.glsl.h"

layout (location = 0) in vec2 v_vertex; // location and uv
layout (location = 1) in uint v_color;
layout (location = 2) in uint v_charcode;
layout (location = 3) in mat4 v_transform;

layout (location = 0) out vec2      f_uv;
layout (location = 1) out uint      f_color;
layout (location = 2) out flat uint f_charcode;

void main() {
    gl_Position = u_viewport_ortho * v_transform * vec4(v_vertex, 0.0f, 1.0f);
    
    f_uv = vec2(v_vertex.x, 1.0f - v_vertex.y); // vertical flip
    f_color = v_color;
    f_charcode = v_charcode;
}
#end vertex

#begin fragment
#version 460 core

#include "color.glsl.h"

layout (location = 0) in vec2      f_uv;
layout (location = 1) in flat uint f_color;
layout (location = 2) in flat uint f_charcode;

layout (location = 0) out vec4 out_color;

uniform sampler2DArray u_sampler_array;

void main() {
    float alpha = texture(u_sampler_array, vec3(f_uv, f_charcode)).r;
    out_color = rgba_unpack(f_color) * vec4(1.0f, 1.0f, 1.0f, alpha);
}
#end fragment
