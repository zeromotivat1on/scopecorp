#include "pch.h"
#include "index_buffer.h"
#include "vertex_buffer.h"
#include "render_registry.h"
#include "gl.h"
#include "log.h"

s32 create_index_buffer(u32* indices, s32 count, Buffer_Usage_Type usage_type)
{
    Index_Buffer buffer;
    buffer.component_count = count;
    buffer.usage_type = usage_type;

    u32 ibo;
    glGenBuffers(1, &ibo);

    s32 gl_usage = 0;
    switch(usage_type)
    {
    case BUFFER_USAGE_STATIC: gl_usage = GL_STATIC_DRAW; break;
    case BUFFER_USAGE_DYNAMIC: gl_usage = GL_DYNAMIC_DRAW; break;
    default:
        error("Failed to create index buffer with unknown usage type %d", usage_type);
        return {0};
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u32), indices, gl_usage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    buffer.id = ibo;
    return add_index_buffer(&render_registry, &buffer);
}
