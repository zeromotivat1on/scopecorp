#pragma once

#include "render_key.h"

struct Shader;
struct Texture;
struct Vertex_Descriptor;
struct Constant_Buffer_Instance;

struct Render_Primitive {
    Shader                            *shader  = null;
    Texture                           *texture = null;
    u32                                vertex_input = 0;
    u64                               *vertex_offsets = null; // per binding in vertex input
    u32                                element_count  = 0;
    u32                                instance_count = 1;
    u32                                first_element  = 0;
    u32                                first_instance = 0;
    enum Gpu_Topology_Mode             topology;
    bool                               indexed   = false;
    bool                               is_entity = false;
    u64                                eid_offset;
    Array <Constant_Buffer_Instance *> cbis = { .allocator = __temporary_allocator };
};

struct Render_Batch_Entry {
    Render_Key render_key;
    Render_Primitive *primitive = null;
};

// A collection of render primitives to be submitted to command buffer for final render.
struct Render_Batch {
    Render_Primitive   *primitives = null;    
    Render_Batch_Entry *entries    = null;

    u32 count    = 0;
    u32 capacity = 0;
};

Render_Batch make_render_batch (u32 capacity, Allocator alc = context.allocator);
void         flush             (Render_Batch *batch);
void         add_primitive     (Render_Batch *batch, const Render_Primitive &prim, Render_Key key = {0});

// Tells whether two given render primitives can be merged into one draw call.
bool can_be_merged (const Render_Primitive   &a, const Render_Primitive   &b);
bool can_be_merged (const Render_Batch_Entry &a, const Render_Batch_Entry &b);

inline bool operator<(const Render_Batch_Entry &a, const Render_Batch_Entry &b) {
    return a.render_key._u64 < b.render_key._u64;
}
