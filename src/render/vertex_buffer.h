#pragma once

#include "render/vertex.h"

inline constexpr s32 MAX_VERTEX_LAYOUT_SIZE    = 8;
inline constexpr s32 MAX_VERTEX_ARRAY_BINDINGS = 8;

enum Buffer_Usage_Type {
	BUFFER_USAGE_STATIC,
	BUFFER_USAGE_DYNAMIC,
	BUFFER_USAGE_STREAM,
};

struct Vertex_Buffer {
	u32 id = 0;
    u32 size = 0;
	Buffer_Usage_Type usage;
};

struct Vertex_Array_Binding {
    s32 vertex_buffer_index = INVALID_INDEX;
    s32 layout_size = 0;
	Vertex_Component layout[MAX_VERTEX_LAYOUT_SIZE];
};

struct Vertex_Array {
    u32 id = 0;
    s32 binding_count = 0;
	Vertex_Array_Binding bindings[MAX_VERTEX_ARRAY_BINDINGS];
};

s32 create_vertex_buffer(const void *data, u32 size, Buffer_Usage_Type usage);
void set_vertex_buffer_data(s32 vertex_buffer_index, const void *data, u32 size, u32 offset);

s32 create_vertex_array(const Vertex_Array_Binding *bindings, s32 binding_count);
s32 vertex_array_vertex_count(s32 vertex_array_index);
