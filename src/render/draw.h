#pragma once

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

// @Cleanup: use handles to render primitives instead of pointers.
struct Draw_Command
{
    u32 flags = 0;
    Draw_Mode draw_mode = DRAW_TRIANGLES;
    
    s32 vertex_buffer_idx = INVALID_INDEX;
    s32 index_buffer_idx  = INVALID_INDEX;
    s32 shader_idx        = INVALID_INDEX;
    s32 texture_idx       = INVALID_INDEX;
    
    s32 instance_count = 1;
};

struct Draw_Queue
{
    Draw_Command* cmds;
    s32 count;
};

inline Draw_Queue draw_queue;

void draw(Draw_Command* cmd);
void init_draw_queue(Draw_Queue* queue);
void enqueue_draw_command(Draw_Queue* queue, Draw_Command* cmd);
void flush_draw_commands(Draw_Queue* queue);
