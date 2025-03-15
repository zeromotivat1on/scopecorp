@vertex_begin
#version 460 core

layout (location = 0) in vec3 v_vertex;
layout (location = 1) in vec2 v_uv;

out vec2 f_uv;

uniform vec2 u_scale;
uniform vec3 u_offset;

void main()
{
    gl_Position = vec4(v_vertex.xyz, 1.0f);
    
    float depth_scale = 1.0f - (u_offset.z * 0.005f);
    vec2 uv = v_uv - 0.5f;
    uv *= depth_scale;
    uv += 0.5f;
    f_uv = (uv + vec2(u_offset) * 0.01f) * u_scale;
}
@vertex_end

@fragment_begin
#version 460 core

in vec2 f_uv;
out vec4 out_color;

uniform sampler2D u_sampler;

void main()
{
    out_color = texture(u_sampler, f_uv);
}
@fragment_end
