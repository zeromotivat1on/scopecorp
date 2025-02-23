#pragma once

#include "math/vector.h"
#include "math/matrix.h"

inline constexpr s32 MAX_UNIFORM_NAME_SIZE  = 64;
inline constexpr s32 MAX_UNIFORM_ARRAY_SIZE = 128; // @Cleanup: grab from system caps

enum Uniform_Type
{
    UNIFORM_NULL,
    UNIFORM_U32,
    UNIFORM_F32,
    UNIFORM_F32_VEC2,
    UNIFORM_F32_VEC3,
    UNIFORM_F32_MAT4,
};

enum Uniform_Flags : u32
{
    UNIFORM_FLAG_DIRTY = 0x1, // have lists of dirty and clean uniforms for fast sync?
};

struct Uniform
{
    char name[MAX_UNIFORM_NAME_SIZE] = {0};
    Uniform_Type type = UNIFORM_NULL;
    u32 flags = 0;
    u32 location = UINT32_MAX;
    s32 count = 1; // greater than 1 for array uniforms
    const void* value = null;
};

Uniform create_uniform(const char* name, Uniform_Type type, s32 count);
void sync_uniform(const Uniform* uniform); // pass uniform data to gfx api if dirty
