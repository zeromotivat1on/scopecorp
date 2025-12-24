#pragma once

struct Material_Info {
    Vector3 ambient; f32 _p0;
    Vector3 diffuse; f32 _p1;
    Vector3 specular;
    f32  shininess;
};

struct Direct_Light_Info {
    Vector3 direction; f32 _p0;

    Vector3 ambient;   f32 _p1;
    Vector3 diffuse;   f32 _p2;
    Vector3 specular;  f32 _p3;
};

struct Point_Light_Info {
    Vector3 position; f32 _p0;
    
    Vector3 ambient;  f32 _p1;
    Vector3 diffuse;  f32 _p2;
    Vector3 specular;

    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
};

struct Global_Parameters {
    Vector4 viewport_resolution;
    Matrix4 viewport_ortho_matrix;

    Vector3 camera_position; f32 _p0;
    Matrix4 camera_view_matrix;
    Matrix4 camera_proj_matrix;
    Matrix4 camera_view_proj_matrix;
};

struct Level_Parameters {
    u32 direct_light_count;
    u32 point_light_count;

    // Max limits obtained from cpu constants with same name.
    Direct_Light_Info direct_lights[MAX_DIRECT_LIGHTS];
    Point_Light_Info  point_lights [MAX_POINT_LIGHTS];
};

struct Entity_Parameters {
    Matrix4 object_to_world;
    Vector2 uv_scale  = Vector2(1.0f);
    Vector3 uv_offset = Vector3(0.0f);
};
