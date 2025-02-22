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
    switch(uniform->type)
    {
    case UNIFORM_F32_VEC2: glUniform2fv(uniform->location, 1, uniform->_vec2.ptr()); break;
    case UNIFORM_F32_VEC3: glUniform3fv(uniform->location, 1, uniform->_vec3.ptr()); break;
    case UNIFORM_F32_MAT4: glUniformMatrix4fv(uniform->location, 1, GL_FALSE, uniform->_mat4.ptr()); break;
    default:
        error("Failed to sync uniform of unknown type %d", uniform->type);
        break;
    }
}
