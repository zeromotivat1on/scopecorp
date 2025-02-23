#include "pch.h"
#include "uniform.h"
#include "gl.h"
#include "log.h"
#include <string.h>

Uniform create_uniform(const char* name, Uniform_Type type, s32 count)
{
    Uniform uniform;
    uniform.type = type;
    uniform.count = count;
    strcpy_s(uniform.name, sizeof(uniform.name), name);
    return uniform;
}

void sync_uniform(const Uniform* uniform)
{
    if (!(uniform->flags & UNIFORM_FLAG_DIRTY)) return;

    switch(uniform->type)
    {
    case UNIFORM_U32:
        glUniform1uiv(uniform->location, uniform->count, (u32*)uniform->value);
        break;
    case UNIFORM_F32_VEC2:
        glUniform2fv(uniform->location, uniform->count, (f32*)uniform->value);
        break;
    case UNIFORM_F32_VEC3:
        glUniform3fv(uniform->location, uniform->count, (f32*)uniform->value);
        break;
    case UNIFORM_F32_MAT4:
        glUniformMatrix4fv(uniform->location, uniform->count, GL_FALSE, (f32*)uniform->value);
        break;
    default:
        error("Failed to sync uniform of unknown type %d", uniform->type);
        break;
    }
}
