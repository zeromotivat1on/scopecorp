[vertex]
#version 460 core

layout (location = 0) in vec3 v_vertex; // xyz | uv

out vec2 f_tex_coords;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(v_vertex.xyz, 1.0f);
    f_tex_coords = v_vertex.xy;
}

[fragment]
#version 460 core

in vec2 f_tex_coords;
out vec4 out_color;

uniform sampler2D u_sampler;

void main()
{
    //out_color = vec4(1, 0, 0, 1);
    out_color = texture(u_sampler, f_tex_coords);
}
