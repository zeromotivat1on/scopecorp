layout (std140) uniform Camera {
    vec3 u_camera_location;
    mat4 u_camera_view;
    mat4 u_camera_proj;
    mat4 u_camera_view_proj;
};
