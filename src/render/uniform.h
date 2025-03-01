#pragma once

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
    UNIFORM_FLAG_DIRTY = 0x1, // have lists of dirty and clean uniforms for faster sync?
};

struct Uniform
{
    Uniform() = default;
    Uniform(const char* name, Uniform_Type type, s32 count)
        : name(name), type(type), count(count) {}
    
    const char* name = null;
    const void* value = null;
    Uniform_Type type = UNIFORM_NULL;
    u32 flags = 0;
    u32 location = MAX_U32;
    s32 count = 0; // greater than 1 for array uniforms
};

void sync_uniform(const Uniform* uniform); // pass uniform data to gpu if dirty
