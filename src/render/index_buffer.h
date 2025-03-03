#pragma once

// @Cleanup: for buffer usage type, move it to common header.
#include "vertex_buffer.h"

struct Index_Buffer {
	u32 id;
	s32 component_count;
	Buffer_Usage_Type usage_type;
};

s32 create_index_buffer(u32 *indices, s32 count, Buffer_Usage_Type usage_type);
