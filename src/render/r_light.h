#pragma once

#include "render/r_uniform.h"

struct alignas(16) U_Direct_Light {
    alignas(16) vec3 direction;
    alignas(16) vec3 ambient;
    alignas(16) vec3 diffuse;
    alignas(16) vec3 specular;
};

struct alignas(16) U_Point_Light {
    alignas(16) vec3 location;
    alignas(16) vec3 ambient;
    alignas(16) vec3 diffuse;
    alignas(16) vec3 specular;

    f32 attenuation_constant  = 0.0f;
    f32 attenuation_linear    = 0.0f;
    f32 attenuation_quadratic = 0.0f;
};

struct R_Direct_Light_Uniform_Block {
    static constexpr String NAME = UNIFORM_BLOCK_NAME_DIRECT_LIGHTS;
    static constexpr u32 BINDING = UNIFORM_BLOCK_BINDING_DIRECT_LIGHTS;
    
    u32 count = 0;
    U_Direct_Light *lights = null;

    Uniform_Block block;
};

struct R_Point_Light_Uniform_Block {
    static constexpr String NAME = UNIFORM_BLOCK_NAME_POINT_LIGHTS;
    static constexpr u32 BINDING = UNIFORM_BLOCK_BINDING_POINT_LIGHTS;
    
    u32 count = 0;
    U_Point_Light *lights = null;

    Uniform_Block block;
};

void r_create(R_Direct_Light_Uniform_Block &ub);
void r_create(R_Point_Light_Uniform_Block  &ub);

void r_add(R_Direct_Light_Uniform_Block &ub, const U_Direct_Light &light);
void r_add(R_Point_Light_Uniform_Block  &ub, const U_Point_Light  &light);

void r_submit(R_Direct_Light_Uniform_Block &ub);
void r_submit(R_Point_Light_Uniform_Block  &ub);
