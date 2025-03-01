#include "pch.h"
#include "render/draw.h"
#include "game/entities.h"

#include "log.h"
#include "memory_storage.h"

void init_draw_queue(Draw_Queue* queue) {
    queue->cmds = alloc_array_persistent(MAX_DRAW_QUEUE_SIZE, Draw_Command);
    queue->count = 0;
}

void enqueue_draw_command(Draw_Queue* queue, const Draw_Command* cmd) {
    assert(queue->count < MAX_DRAW_QUEUE_SIZE);
    const s32 idx = queue->count++;
    memcpy(queue->cmds + idx, cmd, sizeof(Draw_Command));
}

void flush_draw_commands(Draw_Queue* queue) {
    for (s32 i = 0; i < queue->count; ++i) draw(queue->cmds + i);
    queue->count = 0;
}

static Draw_Command draw_command_for(const Entity* e) {
    switch(e->type) {
    case E_STATIC_MESH: {
        const auto* mesh = (Static_Mesh*)e;
        Draw_Command cmd;
        cmd.vertex_buffer_idx = mesh->vertex_buffer_idx;
        cmd.index_buffer_idx  = mesh->index_buffer_idx;
        cmd.material_idx      = mesh->material_idx;
        return cmd;
    }
    case E_PLAYER: {
        const auto* player = (Player*)e;
        Draw_Command cmd;
        cmd.vertex_buffer_idx = player->vertex_buffer_idx;
        cmd.index_buffer_idx  = player->index_buffer_idx;
        cmd.material_idx      = player->material_idx;
        return cmd;
    }
    default:
        error("Failed to create draw command for entity of unknown type %d", e->type);
        return {};
    }
}

void enqueue_draw_entity(Draw_Queue* queue, const Entity* e) {
    const auto cmd = draw_command_for(e);
    enqueue_draw_command(queue, &cmd);
}
