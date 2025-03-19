@vertex_begin
#version 460 core

layout (location = 0) in vec2 v_vertex;
layout (location = 1) in vec2 v_uv;

out vec2 f_uv;

void main()
{
    gl_Position = vec4(v_vertex.xy, 0.0f, 1.0f);
    f_uv = v_uv;
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
