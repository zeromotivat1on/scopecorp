#pragma once

// @Cleanup: for Buffer_Usage_Type enum.
#include "render/vertex_buffer.h"

struct Index_Buffer {
	u32 id;
	s32 index_count;
	Buffer_Usage_Type usage_type;
};

s32 create_index_buffer(const u32 *indices, s32 count, Buffer_Usage_Type usage_type);
