#pragma once

inline s32 R_MAX_VERTEX_ATTRIBUTES;

struct R_Storage {
    rid rid = RID_NONE;
    u32 bits = 0;
    u32 size = 0;
};

struct R_Map_Range {
    const R_Storage *storage = null;
    u32 bits = 0;
    u32 offset = 0;
    u32 size = 0;
    u32 capacity = 0;
    void *data = null;
};

struct R_Alloc_Range {
    R_Map_Range *map = null;
    u32 offset = 0;
    u32 size = 0;
    u32 capacity = 0;
    void *data = null;
};

inline R_Map_Range R_vertex_map_range;
inline R_Map_Range R_index_map_range;

inline R_Alloc_Range R_eid_alloc_range;

void          r_create_storage(u32 size, u32 bits, R_Storage &storage);
R_Map_Range   r_map(R_Storage &storage, u32 offset, u32 size, u32 bits);
bool          r_unmap(R_Storage &storage); // all mapped ranges will become invalid
//void          r_flush(const R_Map_Range &map);
R_Alloc_Range r_alloc(R_Map_Range &map, u32 size);
void         *r_alloc(R_Alloc_Range &alloc, u32 size);

u32 head_pointer(R_Alloc_Range &alloc);
