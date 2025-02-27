[vertex]
#version 460 core

layout (location = 0) in vec3 v_vertex;
layout (location = 1) in vec2 v_uv;

out vec2 f_uv;

uniform mat4 u_mvp;
uniform vec2 u_scale;

void main()
{
    gl_Position = u_mvp * vec4(v_vertex.xyz, 1.0f);
    f_uv = v_uv * u_scale;
}

[fragment]
#version 460 core

in vec2 f_uv;
out vec4 out_color;

uniform sampler2D u_sampler;

void main()
{
    //out_color = vec4(1.0f);
    out_color = texture(u_sampler, f_uv);
}
