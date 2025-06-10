#pragma once

#include "asset.h"

#include "math/vector.h"

#include "render/uniform.h"

inline constexpr u32 MAX_MATERIAL_SIZE     = KB(4);
inline constexpr s32 MAX_MATERIAL_UNIFORMS = 16;

struct Render_Command;

struct Material : Asset {
    Material() { asset_type = ASSET_MATERIAL; }
    
    sid sid_shader  = SID_NONE;
    sid sid_texture = SID_NONE;
    
    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_black;
    f32  shininess = 1.0f;

    s32 uniform_count = 0;
    Uniform uniforms[MAX_MATERIAL_UNIFORMS];
};

void init_material_asset(Material *material, void *data);
void set_material_uniform_value(Material *material, const char *uniform_name, const void *data, u32 size, u32 offset = 0);
