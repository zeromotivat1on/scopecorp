#pragma once

#include "vertex.h"

enum Buffer_Usage_Type {
	BUFFER_USAGE_STATIC,
	BUFFER_USAGE_DYNAMIC,
	BUFFER_USAGE_STREAM,
};

struct Vertex_Buffer {
	u32 id; // vertex array for opengl
	s32 vertex_count;
	Buffer_Usage_Type usage_type;
	Vertex_Layout layout;
};

s32 create_vertex_buffer(Vertex_Attrib_Type *attribs, s32 attrib_count, const void *data, s32 vertex_count, Buffer_Usage_Type usage_type);
