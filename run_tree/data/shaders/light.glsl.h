#define MAX_DIRECT_LIGHTS 4  // must be the same as in game code
#define MAX_POINT_LIGHTS  32 // must be the same as in game code

layout (std140) uniform Direct_Lights {
    uint u_direct_light_count;
    
    vec3 u_direct_light_directions[MAX_DIRECT_LIGHTS];
    
    vec3 u_direct_light_ambients [MAX_DIRECT_LIGHTS];
    vec3 u_direct_light_diffuses [MAX_DIRECT_LIGHTS];
    vec3 u_direct_light_speculars[MAX_DIRECT_LIGHTS];
};

layout (std140) uniform Point_Lights {
    uint u_point_light_count;
    
    vec3 u_point_light_locations[MAX_POINT_LIGHTS];
    
    vec3 u_point_light_ambients [MAX_POINT_LIGHTS];
    vec3 u_point_light_diffuses [MAX_POINT_LIGHTS];
    vec3 u_point_light_speculars[MAX_POINT_LIGHTS];

    float u_point_light_attenuation_constants [MAX_POINT_LIGHTS];
    float u_point_light_attenuation_linears   [MAX_POINT_LIGHTS];
    float u_point_light_attenuation_quadratics[MAX_POINT_LIGHTS];
};

vec3 get_direct_light(vec3 normal, vec3 view_direction,
                      vec3 l_direction, vec3 l_ambient, vec3 l_diffuse, vec3 l_specular,
                      vec3 m_ambient, vec3 m_diffuse, vec3 m_specular, float m_shininess) {
    l_direction = normalize(-l_direction);

    const vec3 ambient = l_ambient * m_ambient;
    
    const float diffuse_factor = max(dot(normal, l_direction), 0.0);
    const vec3 diffuse = l_diffuse * diffuse_factor * m_diffuse;
    
    const vec3 l_reflected_direction = reflect(-l_direction, normal);
    const float specular_factor = pow(max(dot(view_direction, l_reflected_direction), 0.0f), m_shininess);
    const vec3 specular = l_specular * specular_factor * m_specular;

    return ambient + diffuse + specular;
}

vec3 get_point_light(vec3 normal, vec3 view_direction, vec3 pixel_world_location,
                     vec3 l_location, vec3 l_ambient, vec3 l_diffuse, vec3 l_specular,
                     float l_att_constant, float l_att_linear, float l_att_quadratic,
                     vec3 m_ambient, vec3 m_diffuse, vec3 m_specular, float m_shininess) {
    const vec3 l_direction = normalize(l_location - pixel_world_location);

    vec3 ambient = l_ambient * m_ambient;

    const float diffuse_factor = max(dot(normal, l_direction), 0.0f);
    vec3 diffuse = l_diffuse * (diffuse_factor * m_diffuse);

    const vec3 l_reflected_direction = reflect(-l_direction, normal);
    const float specular_factor = pow(max(dot(view_direction, l_reflected_direction), 0.0f), m_shininess);
    vec3 specular = l_specular * specular_factor * m_specular;

    const float distance = length(l_location - pixel_world_location);
    const float attenuation = 1.0f / (l_att_constant + l_att_linear * distance + l_att_quadratic * distance * distance);

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}
