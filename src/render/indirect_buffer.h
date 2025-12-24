#pragma once

struct Indirect_Draw_Command {
    u32 vertex_count;
    u32 instance_count;
    u32 first_vertex;
    u32 first_instance;
};

struct Indirect_Draw_Indexed_Command {
    u32 index_count;
    u32 instance_count;
    u32 first_index;
    s32 vertex_offset;
    u32 first_instance;
};

// Indirect buffer stores indirect commands both for vertex and index draw,
// its up to the client to store, interpret and use data correctly.
struct Indirect_Buffer {
    void *data;
    u32   size;
    u32   used;
};

auto make_indirect_buffer (u32 size, Allocator alc) -> Indirect_Buffer;
void reset                (Indirect_Buffer *buffer);
void add_command          (Indirect_Buffer *buffer, const Indirect_Draw_Command         &cmd);
void add_command          (Indirect_Buffer *buffer, const Indirect_Draw_Command         &cmd);
void add_command          (Indirect_Buffer *buffer, const Indirect_Draw_Indexed_Command &cmd);
