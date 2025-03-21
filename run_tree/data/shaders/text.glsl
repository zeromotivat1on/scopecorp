#begin vertex
#version 460 core

layout (location = 0) in vec2 v_vertex; // location and uv

layout (location = 0) out vec2     f_uv;
layout (location = 1) out flat int f_instance_id;

uniform mat4 u_transforms[128];
uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * u_transforms[gl_InstanceID] * vec4(v_vertex, 0.0f, 1.0f);
    
    f_uv = vec2(v_vertex.x, 1.0f - v_vertex.y); // vertical flip
    f_instance_id = gl_InstanceID;
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec2     f_uv;
layout (location = 1) in flat int f_instance_id;

layout (location = 0) out vec4 out_color;

uniform sampler2DArray u_sampler_array;
uniform unsigned int   u_charmap[128];
uniform vec3           u_color;

void main() {
    float sampled_alpha = texture(u_sampler_array, vec3(f_uv, u_charmap[f_instance_id])).r;
    vec4 sampled = vec4(1.0f, 1.0f, 1.0f, sampled_alpha);
    out_color = vec4(u_color, 1.0f) * sampled;
}
#end fragment
