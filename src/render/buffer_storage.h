#pragma once

#include "render/vertex.h"

inline s32 R_MAX_VERTEX_ATTRIBUTES;

extern u32 R_USAGE_DRAW_STATIC;
extern u32 R_USAGE_DRAW_DYNAMIC;
extern u32 R_USAGE_DRAW_STREAM;

extern u32 R_FLAG_MAP_READ;
extern u32 R_FLAG_MAP_WRITE;
extern u32 R_FLAG_MAP_PERSISTENT;
extern u32 R_FLAG_MAP_COHERENT;

extern u32 R_FLAG_STORAGE_DYNAMIC;
extern u32 R_FLAG_STORAGE_CLIENT;

inline constexpr s32 MAX_VERTEX_LAYOUT_SIZE    = 8;
inline constexpr s32 MAX_VERTEX_ARRAY_BINDINGS = 8;

inline constexpr u32 MAX_VERTEX_STORAGE_SIZE = MB(16);
inline constexpr u32 MAX_INDEX_STORAGE_SIZE  = MB(4);

inline constexpr u32 MAX_EID_VERTEX_DATA_SIZE = MB(1);
inline constexpr s32 EID_VERTEX_BINDING_INDEX = MAX_VERTEX_ARRAY_BINDINGS;
inline u32 EID_VERTEX_DATA_OFFSET = 0;
inline u32 EID_VERTEX_DATA_SIZE   = 0;
inline void *EID_VERTEX_DATA = null;

struct Buffer_Storage {
    rid rid = RID_NONE;
    u32 size = 0;
    u32 capacity = 0;
    void *mapped_data = null;
};

struct Vertex_Buffer {
	rid rid = RID_NONE;
    u32 size = 0;
	u32 usage = 0;
};

struct Storage_Data_Range {
    rid rid = RID_NONE;
    u32 offset = 0;
    u32 size   = 0;
};

struct Vertex_Array_Binding {
    s32 binding_index = INVALID_INDEX;
    u32 data_offset = 0;
    s32 layout_size = 0;
	Vertex_Component layout[MAX_VERTEX_LAYOUT_SIZE];
};

struct Vertex_Array {
    rid rid = RID_NONE;
    s32 binding_count = 0;
	Vertex_Array_Binding bindings[MAX_VERTEX_ARRAY_BINDINGS];
};

struct Index_Buffer {
	rid rid = RID_NONE;
	u32 index_count = 0;
	u32 usage = 0;
};

inline Buffer_Storage vertex_buffer_storage;
inline Buffer_Storage index_buffer_storage;

rid r_create_storage(const void *data, u32 size, u32 flags); // storage and map flags
rid r_create_buffer(const void *data, u32 size, u32 usage);
rid r_create_vertex_array(const Vertex_Array_Binding *bindings, s32 binding_count);
void *r_map_buffer(rid rid, u32 offset, u32 size, u32 flags);
bool r_unmap_buffer(rid rid);
void r_flush_buffer(rid rid, u32 offset, u32 size);

void *r_allocv(u32 size); // allocate from vertex storage
void *r_alloci(u32 size); // allocate from index storage

void r_init_buffer_storages();

s32 create_vertex_buffer(const void *data, u32 size, u32 usage);
void set_vertex_buffer_data(s32 vertex_buffer_index, const void *data, u32 size, u32 offset);

s32 create_vertex_array(const Vertex_Array_Binding *bindings, s32 binding_count);
s32 get_vertex_array_vertex_count(s32 vertex_array_index);

s32 create_index_buffer(const u32 *indices, u32 count, u32 usage);
