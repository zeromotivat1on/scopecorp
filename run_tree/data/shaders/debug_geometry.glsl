@vertex_begin
#version 460 core

layout (location = 0) in vec3 v_location;
layout (location = 1) in vec3 v_color;

out vec3 f_color;

uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * vec4(v_location, 1.0f);
    f_color = v_color;
}
@vertex_end

@fragment_begin
#version 460 core

in vec3 f_color;
out vec4 out_color;

void main() {
    out_color = vec4(f_color, 1.0f);
}
@fragment_end