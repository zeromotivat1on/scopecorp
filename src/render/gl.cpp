#include "pch.h"
#include "gl.h"
#include "log.h"

u32 gl_create_shader(GLenum type, const char* src)
{    
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, null);
    glCompileShader(shader);

    s32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        const char* shader_name;
        if (type == GL_VERTEX_SHADER) shader_name = "vertex";
        else if (type == GL_FRAGMENT_SHADER) shader_name = "fragment";

        char info_log[512];
        glGetShaderInfoLog(shader, sizeof(info_log), null, info_log);
        error("Failed to compile shader %s, gl reason %s", shader_name, info_log);
        return INVALID_INDEX;
    }

    return shader;
}

u32 gl_link_program(u32 vertex_shader, u32 fragment_shader)
{    
    u32 program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    s32 success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetProgramInfoLog(program, sizeof(info_log), null, info_log);
        error("Failed to link shader program, gl reason %s", info_log);
        return INVALID_INDEX;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

u32 gl_create_texture(void* data, s32 width, s32 height)
{
    u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}
