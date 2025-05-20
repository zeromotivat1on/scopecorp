#define MAX_DIRECT_LIGHTS 4  // must be the same as in game code
#define MAX_POINT_LIGHTS  32 // must be the same as in game code

layout (std140) uniform Camera {
    vec3 u_camera_location;
    mat4 u_view;
    mat4 u_proj;
    mat4 u_view_proj;
};

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
