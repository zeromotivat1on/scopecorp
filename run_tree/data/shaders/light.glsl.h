struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
};

struct Direct_Light {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Point_Light {
    vec3 location;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
};

layout (std140) uniform Direct_Lights {
    #define MAX_DIRECT_LIGHTS 4  // must be the same as in game code

    uint u_direct_light_count;
    Direct_Light u_direct_lights[MAX_DIRECT_LIGHTS];
};

layout (std140) uniform Point_Lights {
    #define MAX_POINT_LIGHTS  32 // must be the same as in game code

    uint u_point_light_count;
    Point_Light u_point_lights[MAX_POINT_LIGHTS];
};

vec3 get_direct_light(vec3 normal, vec3 view_direction, Direct_Light light, Material mat) {
    light.direction = normalize(-light.direction);

    const vec3 ambient = light.ambient * mat.ambient;
    
    const float diffuse_factor = max(dot(normal, light.direction), 0.0);
    const vec3 diffuse = light.diffuse * diffuse_factor * mat.diffuse;
    
    const vec3 reflected_direction = reflect(-light.direction, normal);
    const float specular_factor = pow(max(dot(view_direction, reflected_direction), 0.0f), mat.shininess);
    const vec3 specular = light.specular * specular_factor * mat.specular;

    return ambient + diffuse + specular;
}

vec3 get_point_light(vec3 normal, vec3 view_direction, vec3 pixel_world_location,
                     Point_Light light, Material mat) {
    const vec3 light_direction = normalize(light.location - pixel_world_location);

    vec3 ambient = light.ambient * mat.ambient;

    const float diffuse_factor = max(dot(normal, light_direction), 0.0f);
    vec3 diffuse = light.diffuse * (diffuse_factor * mat.diffuse);

    const vec3 reflected_direction = reflect(-light_direction, normal);
    const float specular_factor = pow(max(dot(view_direction, reflected_direction), 0.0f), mat.shininess);
    vec3 specular = light.specular * specular_factor * mat.specular;

    const float distance = length(light.location - pixel_world_location);
    const float attenuation = 1.0f / (light.attenuation_constant + light.attenuation_linear * distance + light.attenuation_quadratic * distance * distance);

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}
