#include "pch.h"
#include "draw.h"
#include "gl.h"
#include "log.h"
#include "shader.h"
#include "texture.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "render_registry.h"
#include <string.h>

void draw(Draw_Command* cmd)
{
    assert(cmd->vertex_buffer_idx < MAX_VERTEX_BUFFERS);
    assert(cmd->index_buffer_idx < MAX_INDEX_BUFFERS);
    assert(cmd->shader_idx < MAX_SHADERS);
    assert(cmd->texture_idx < MAX_TEXTURES);

    s32 gl_draw_mode = -1;
    switch(cmd->draw_mode)
    {
        case DRAW_TRIANGLES:      gl_draw_mode = GL_TRIANGLES; break;
        case DRAW_TRIANGLE_STRIP: gl_draw_mode = GL_TRIANGLE_STRIP; break;
        default:
            error("Failed to draw, unknown draw mode %d", cmd->draw_mode);
            return;
    }
    
    if (cmd->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_FALSE);

    const auto* shader = render_registry.shaders + cmd->shader_idx;
    glUseProgram(shader->id);

    for (s32 i = 0; i < shader->uniform_count; ++i)
        sync_uniform(shader->uniforms + i);
    
    if (cmd->texture_idx != INVALID_INDEX)
    {
        const auto* texture = render_registry.textures + cmd->texture_idx;
        if (texture->flags & TEXTURE_FLAG_2D_ARRAY) glBindTexture(GL_TEXTURE_2D_ARRAY, texture->id);
        else glBindTexture(GL_TEXTURE_2D, texture->id);
        glActiveTexture(GL_TEXTURE0);
    }

    const auto* vertex_buffer = render_registry.vertex_buffers + cmd->vertex_buffer_idx;
    glBindVertexArray(vertex_buffer->id);

    if (cmd->index_buffer_idx != INVALID_INDEX)
    {
        const auto* index_buffer = render_registry.index_buffers + cmd->index_buffer_idx;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->id);
        glDrawElementsInstanced(gl_draw_mode, index_buffer->component_count, GL_UNSIGNED_INT, 0, cmd->instance_count);
    }
    else
    {
        glDrawArraysInstanced(gl_draw_mode, 0, vertex_buffer->component_count, cmd->instance_count);
    }
    
    if (cmd->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_TRUE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void init_draw_queue(Draw_Queue* queue)
{
    queue->cmds = alloc_array_persistent(MAX_DRAW_QUEUE_SIZE, Draw_Command);
    queue->count = 0;
}

void enqueue_draw_command(Draw_Queue* queue, Draw_Command* cmd)
{
    assert(queue->count < MAX_DRAW_QUEUE_SIZE);

    const s32 idx = queue->count++;
    memcpy(queue->cmds + idx, cmd, sizeof(Draw_Command));
}

void flush_draw_commands(Draw_Queue* queue)
{
    for (s32 i = 0; i < queue->count; ++i) draw(queue->cmds + i);
    queue->count = 0;
}
