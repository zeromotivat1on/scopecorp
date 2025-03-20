@vertex_begin
#version 460 core

layout (location = 0) in vec3 v_location;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in int  v_entity_id;

layout (location = 0) out vec2     f_uv;
layout (location = 1) out flat int f_entity_id;

uniform mat4 u_transform;
uniform vec2 u_uv_scale;

void main() {
    gl_Position = u_transform * vec4(v_location, 1.0f);
    
    f_uv = v_uv * u_uv_scale;
    f_entity_id = v_entity_id;
}
@vertex_end

@fragment_begin
#version 460 core

layout (location = 0) in vec2     f_uv;
layout (location = 1) in flat int f_entity_id;

layout (location = 0) out vec4 out_color;
layout (location = 1) out int  out_entity_id;

uniform sampler2D u_sampler;

void main() {
    //out_color = vec4(1, 0, 0, 1);
    out_color = texture(u_sampler, f_uv);
    out_entity_id = f_entity_id;
}
@fragment_end
