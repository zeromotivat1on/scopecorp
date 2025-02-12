[vertex]
#version 460 core

layout (location = 0) in vec2 v_vertex; // vec2 pos

out vec2 f_tex_coords;
out flat int f_instance_id;

uniform mat4 u_transforms[128];
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_transforms[gl_InstanceID] * vec4(v_vertex.xy, 0.0f, 1.0f);
    f_tex_coords = v_vertex.xy;
    f_tex_coords.y = 1.0f - v_vertex.y; // vertical flip
    f_instance_id = gl_InstanceID;
}

[fragment]
#version 460 core

in vec2 f_tex_coords;
in flat int f_instance_id;
out vec4 out_color;

uniform sampler2DArray u_text_sampler_array;
uniform unsigned int u_charmap[128];
uniform vec3 u_text_color;

void main()
{
    vec4 sampled = vec4(1.0f, 1.0f, 1.0f, texture(u_text_sampler_array, vec3(f_tex_coords.xy, u_charmap[f_instance_id])).r);
    out_color = vec4(u_text_color, 1.0f) * sampled;
}
