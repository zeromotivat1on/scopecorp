#include "pch.h"
#include "draw.h"
#include "gl.h"
#include "shader.h"
#include "texture.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "render_registry.h"

void draw(Draw_Command* cmd)
{
    assert(cmd->vertex_buffer_idx < MAX_VERTEX_BUFFERS);
    assert(cmd->index_buffer_idx < MAX_INDEX_BUFFERS);
    assert(cmd->shader_idx < MAX_SHADERS);
    assert(cmd->texture_idx < MAX_TEXTURES);

    if (cmd->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_FALSE);

    const auto* shader = render_registry.shaders + cmd->shader_idx;
    glUseProgram(shader->id);

    for (s32 i = 0; i < shader->uniform_count; ++i)
        sync_uniform(shader->uniforms + i);
    
    if (cmd->texture_idx != INVALID_INDEX)
    {
        const auto* texture = render_registry.textures + cmd->texture_idx;
        glBindTexture(GL_TEXTURE_2D, texture->id);
        glActiveTexture(GL_TEXTURE0);
    }

    const auto* vertex_buffer = render_registry.vertex_buffers + cmd->vertex_buffer_idx;
    glBindVertexArray(vertex_buffer->id);

    if (cmd->index_buffer_idx != INVALID_INDEX)
    {
        const auto* index_buffer = render_registry.index_buffers + cmd->index_buffer_idx;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->id);
        glDrawElements(GL_TRIANGLES, index_buffer->component_count, GL_UNSIGNED_INT, 0);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, vertex_buffer->component_count);
    }
    
    if (cmd->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_TRUE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}
