#pragma once

#include "gpu.h"
#include "render_batch.h"

#ifndef RENDER_FRAMES_IN_FLIGHT
#define RENDER_FRAMES_IN_FLIGHT 3
#endif

struct Render_Frame {
    u32 draw_call_count = 0;

    Render_Batch opaque_batch;
    Render_Batch transparent_batch;
    Render_Batch hud_batch;

    Handle         syncs                   [RENDER_FRAMES_IN_FLIGHT];
    u32            command_buffers         [RENDER_FRAMES_IN_FLIGHT];
    Gpu_Allocation gpu_indirect_allocations[RENDER_FRAMES_IN_FLIGHT];
    Gpu_Allocation gpu_submit_allocations  [RENDER_FRAMES_IN_FLIGHT];
};

void init_render_frame   ();
void post_render_cleanup ();
void render_one_frame    ();

void render_entity (struct Entity *e);

u32             get_draw_call_count   ();
void            inc_draw_call_count   ();
void            reset_draw_call_count ();
Handle         *get_render_frame_sync ();
Render_Batch   *get_opaque_batch      ();
Render_Batch   *get_transparent_batch ();
Render_Batch   *get_hud_batch         ();
u32             get_command_buffer    ();
Gpu_Allocation *get_gpu_indirect_allocation ();
Gpu_Allocation *get_gpu_submit_allocation   ();
