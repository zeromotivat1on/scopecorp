#begin vertex
#version 460 core

#include "uniform_blocks.glsl.h"

layout (location = 0) in vec3 v_location;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in int  v_entity_id;

layout (location = 0) out vec3     f_normal;
layout (location = 1) out vec2     f_uv;
layout (location = 2) out flat int f_entity_id;
layout (location = 3) out vec3     f_pixel_world_location;

uniform mat4 u_model;
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

#include "uniform_blocks.glsl.h"
#include "light.glsl.h"

layout (location = 0) in vec3     f_normal;
layout (location = 1) in vec2     f_uv;
layout (location = 2) in flat int f_entity_id;
layout (location = 3) in vec3     f_pixel_world_location;

layout (location = 0) out vec4 out_color;
layout (location = 1) out int  out_entity_id;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

uniform sampler2D u_sampler;
uniform Material  u_material;

void main() {
    const vec3 normal = vec3(0, 1, 0);
    const vec3 view_direction = normalize(u_camera_location - f_pixel_world_location);

    vec3 phong;

    for (int i = 0; i < u_direct_light_count; ++i) {
        phong += get_direct_light(normal, view_direction,
                                  u_direct_light_directions[i], u_direct_light_ambients[i],
                                  u_direct_light_diffuses[i], u_direct_light_speculars[i],
                                  u_material.ambient, u_material.diffuse,
                                  u_material.specular, u_material.shininess);
    }

    for (int i = 0; i < u_point_light_count; ++i) {
        phong += get_point_light(normal, view_direction, f_pixel_world_location,
                                 u_point_light_locations[i], u_point_light_ambients[i],
                                 u_point_light_diffuses[i], u_point_light_speculars[i],
                                 u_point_light_attenuation_constants[i],
                                 u_point_light_attenuation_linears[i],
                                 u_point_light_attenuation_quadratics[i],
                                 u_material.ambient, u_material.diffuse,
                                 u_material.specular, u_material.shininess);
    }

    //out_color = vec4(1, 0, 0, 1) * vec4(phong, 1.0f);
    out_color = texture(u_sampler, f_uv) * vec4(phong, 1.0f);
    out_entity_id = f_entity_id;
}
#end fragment
