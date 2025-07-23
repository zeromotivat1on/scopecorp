#pragma once

#include "math/vector.h"
#include "render/r_uniform.h"

inline constexpr u32 MAX_MATERIAL_SIZE     = KB(4);
inline constexpr s32 MAX_MATERIAL_UNIFORMS = 16;

struct R_Light_Params {
    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_black;
    f32  shininess = 1.0f;
};

struct R_Material {
    static constexpr u32 MAX_FILE_SIZE = KB(64);
    static constexpr u32 MAX_UNIFORMS  = 16;
    
    u16 shader  = 0;    
    u16 texture = 0;
    
    R_Light_Params light_params;

    u16 uniform_count = 0;
    u16 uniforms[MAX_UNIFORMS] = { 0 };

    struct Meta {
        sid shader  = SID_NONE;
        sid texture = SID_NONE;

        R_Light_Params light_params;

        u16 uniform_count = 0;
        Uniform uniforms[MAX_UNIFORMS] = { 0 }; // @Cleanup: extra memory is written to pak.
    };
};

u16  r_create_material(u16 shader, u16 texture, R_Light_Params lparams, u16 ucount, const u16 *uniforms);
void r_set_material_uniform(u16 material, sid name, u32 offset, u32 size, const void *data);
