#include "pch.h"
#include "vertex_buffer.h"
#include "render_registry.h"
#include "gl.h"
#include "log.h"
#include <string.h>

s32 create_vertex_buffer(Vertex_Attrib_Type* attribs, s32 attrib_count, const f32* vertices, s32 vertex_count, Buffer_Usage_Type usage_type)
{
    assert(attrib_count <= MAX_VERTEX_LAYOUT_ATTRIBS);

    Vertex_Buffer buffer = {0};
    buffer.usage_type = usage_type;
    buffer.component_count = vertex_count;
    memcpy(&buffer.layout.attribs, attribs, attrib_count * sizeof(Vertex_Attrib_Type));

    u32 vbo;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &buffer.id);

    glBindVertexArray(buffer.id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    s32 gl_usage = 0;
    switch(usage_type)
    {
    case BUFFER_USAGE_STATIC: gl_usage = GL_STATIC_DRAW; break;
    case BUFFER_USAGE_DYNAMIC: gl_usage = GL_DYNAMIC_DRAW; break;
    default:
        error("Failed to create vertex buffer with unknown usage type %d", usage_type);
        return {0};
    }

    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(f32), vertices, gl_usage);
    
    s32 stride = 0;
    for (s32 i = 0; i < attrib_count; ++i)
        stride += vertex_attrib_size(attribs[i]);
        
    for (s32 i = 0; i < attrib_count; ++i)
    {
        const s32 dimension = vertex_attrib_dimension(attribs[i]);
        
        s32 offset = 0;
        for (s32 j = 0; j < i; ++j)
            offset += vertex_attrib_size(attribs[j]);

        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, dimension, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return add_vertex_buffer(&render_registry, &buffer);
}
