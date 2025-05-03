#begin vertex
#version 460 core

layout (location = 0) in vec3 v_location;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in int  v_entity_id;

layout (location = 0) out vec3     f_normal;
layout (location = 1) out vec2     f_uv;
layout (location = 2) out flat int f_entity_id;
layout (location = 3) out vec3     f_pixel_world_location;

uniform mat4 u_model;
uniform mat4 u_view_proj;
uniform vec2 u_uv_scale;

void main() {
    gl_Position = u_view_proj * u_model * vec4(v_location, 1.0f);

    f_normal = v_normal;
    f_uv = v_uv * u_uv_scale;
    f_entity_id = v_entity_id;
    f_pixel_world_location = vec3(u_model * vec4(v_location, 1.0f));
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec3     f_normal;
layout (location = 1) in vec2     f_uv;
layout (location = 2) in flat int f_entity_id;
layout (location = 3) in vec3     f_pixel_world_location;

layout (location = 0) out vec4 out_color;
layout (location = 1) out int  out_entity_id;

struct Light {
    vec3 location;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

uniform sampler2D u_sampler;
uniform vec3      u_camera_location;
uniform Light     u_light;
uniform Material  u_material;

void main() {
    const vec3 ambient = u_light.ambient * u_material.ambient;

    const vec3 normal = normalize(f_normal);
    const vec3 light_direction = normalize(u_light.location - f_pixel_world_location);
    const float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    const vec3 diffuse = u_light.diffuse * (diffuse_factor * u_material.diffuse);

    float specular_strength = 0.1f;
    const vec3 view_direction = normalize(u_camera_location - f_pixel_world_location);
    const vec3 reflected_direction = reflect(-light_direction, normal);
    const float specular_factor = pow(max(dot(view_direction, reflected_direction), 0.0f), u_material.shininess);
    const vec3 specular = u_light.specular * (specular_factor * u_material.specular);

    const vec3 phong = ambient + diffuse + specular;
    //out_color = vec4(1, 0, 0, 1) * vec4(phong, 1.0f);
    out_color = texture(u_sampler, f_uv) * vec4(phong, 1.0f);
    out_entity_id = f_entity_id;
}
#end fragment
