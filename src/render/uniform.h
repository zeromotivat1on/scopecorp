#pragma once

#include "math/vector.h"
#include "math/matrix.h"

enum Uniform_Type
{
    UNIFORM_NULL,
    UNIFORM_F32_VEC2,
    UNIFORM_F32_VEC3,
    UNIFORM_F32_MAT4,
};

struct Uniform
{
    char name[64] = {0};
    Uniform_Type type = UNIFORM_NULL;
    u32 location = UINT32_MAX;
    s32 count = 1; // greater than 1 for array uniforms

    // C++ can't default construct these, despite they are default constructible...
    // Even if compiler does not know what to construct, its not possible to just
    // mark desired member that should be constructed by default.
    union
    {
        vec2 _vec2;
        vec3 _vec3;
        mat4 _mat4;
    };

    // So we must init union in constructor...
    Uniform() : _mat4() {}
};

Uniform create_uniform(const char* name, Uniform_Type type, s32 count);
void sync_uniform(const Uniform* uniform);
