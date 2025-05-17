#define MAX_LIGHTS 64

layout (std140) uniform Camera {
    vec3 u_camera_location;
    mat4 u_view;
    mat4 u_proj;
    mat4 u_view_proj;
};

layout (std140) uniform Lights {
    uint u_light_count;
    vec3 u_light_locations[MAX_LIGHTS];
    vec3 u_light_ambients [MAX_LIGHTS];
    vec3 u_light_diffuses [MAX_LIGHTS];
    vec3 u_light_speculars[MAX_LIGHTS];
};
