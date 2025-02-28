#pragma once

// @Cleanup: includes for inline functions down below, remove later.
#include "memory_storage.h"
#include <string.h>

inline constexpr s32 MAX_DRAW_QUEUE_SIZE = 64;

enum Draw_Mode
{
    DRAW_TRIANGLES,
    DRAW_TRIANGLE_STRIP,
};

enum Draw_Command_Flags : u32
{
    DRAW_FLAG_IGNORE_DEPTH = 0x1,
};

struct Draw_Command
{
    u32 flags = 0;
    Draw_Mode draw_mode = DRAW_TRIANGLES;
    
    s32 vertex_buffer_idx = INVALID_INDEX;
    s32 index_buffer_idx  = INVALID_INDEX;
    s32 material_idx      = INVALID_INDEX;
    
    s32 instance_count = 1;
};

struct Draw_Queue
{
    Draw_Command* cmds;
    s32 count;
};

inline Draw_Queue draw_queue;

void draw(const Draw_Command* cmd);

// @Cleanup: find home for these.
inline void init_draw_queue(Draw_Queue* queue) {
    queue->cmds = alloc_array_persistent(MAX_DRAW_QUEUE_SIZE, Draw_Command);
    queue->count = 0;
}

inline void enqueue_draw_command(Draw_Queue* queue, Draw_Command* cmd) {
    assert(queue->count < MAX_DRAW_QUEUE_SIZE);
    const s32 idx = queue->count++;
    memcpy(queue->cmds + idx, cmd, sizeof(Draw_Command));
}

inline void flush_draw_commands(Draw_Queue* queue) {
    for (s32 i = 0; i < queue->count; ++i) draw(queue->cmds + i);
    queue->count = 0;
}
