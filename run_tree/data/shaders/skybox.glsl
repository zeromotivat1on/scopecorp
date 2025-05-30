#begin vertex
#version 460 core

layout (location = 0) in vec3 v_location;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in int  v_entity_id;

layout (location = 0) out vec2     f_uv;
layout (location = 1) out flat int f_entity_id;

uniform vec2 u_scale;
uniform vec3 u_offset;

void main() {
    gl_Position = vec4(v_location, 1.0f);
    
    float depth_scale = 1.0f - (u_offset.z * 0.005f);
    vec2 uv = v_uv - 0.5f;
    uv *= depth_scale;
    uv += 0.5f;
    f_uv = (uv + vec2(u_offset) * 0.01f) * u_scale;

    f_entity_id = v_entity_id;
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec2 f_uv;
layout (location = 1) in flat int f_entity_id;

layout (location = 0) out vec4 out_color;
layout (location = 1) out int  out_entity_id;

uniform sampler2D u_sampler;

void main() {
    out_color = vec4(0, 0, 0, 1);
    //out_color = texture(u_sampler, f_uv);
    out_entity_id = f_entity_id;
}
#end fragment
