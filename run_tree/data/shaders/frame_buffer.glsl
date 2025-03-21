#begin vertex
#version 460 core

layout (location = 0) in vec2 v_location;
layout (location = 1) in vec2 v_uv;

layout (location = 0) out vec2 f_uv;

void main() {
    gl_Position = vec4(v_location, 0.0f, 1.0f);
    f_uv = v_uv;
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec2 f_uv;

layout (location = 0) out vec4 out_color;

uniform sampler2D u_sampler;

void main() {
    out_color = texture(u_sampler, f_uv);
}
#end fragment
