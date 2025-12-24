#pragma once

#include "gpu.h"
#include "render_batch.h"
#include "command_buffer.h"
#include "indirect_buffer.h"
#include "wrap_buffer.h"

#ifndef RENDER_FRAMES_IN_FLIGHT
#define RENDER_FRAMES_IN_FLIGHT 3
#endif

struct Render_Frame {
    u32 draw_call_count = 0;

    Render_Batch opaque_batch;
    Render_Batch transparent_batch;
    Render_Batch hud_batch;

    Gpu_Handle      syncs           [RENDER_FRAMES_IN_FLIGHT];
    Command_Buffer  command_buffers [RENDER_FRAMES_IN_FLIGHT];
    Indirect_Buffer indirect_buffers[RENDER_FRAMES_IN_FLIGHT];
    Wrap_Buffer     submit_buffers  [RENDER_FRAMES_IN_FLIGHT];
};

void init_render_frame   ();
void post_render_cleanup ();
void render_one_frame    ();

void render_entity (struct Entity *e);

u32              get_draw_call_count   ();
void             inc_draw_call_count   ();
void             reset_draw_call_count ();
Gpu_Handle      *get_render_frame_sync ();
Render_Batch    *get_opaque_batch      ();
Render_Batch    *get_transparent_batch ();
Render_Batch    *get_hud_batch         ();
Command_Buffer  *get_command_buffer    ();
Indirect_Buffer *get_indirect_buffer   ();
Wrap_Buffer     *get_submit_buffer     ();
