#include "pch.h"
#include "vertex.h"
#include "log.h"

s32 vertex_component_dimension(Vertex_Component_Type type) {
	switch (type) {
	case VERTEX_S32:   return 1;
	case VERTEX_U32:   return 1;
	case VERTEX_F32_2: return 2;
	case VERTEX_F32_3: return 3;
	case VERTEX_F32_4: return 4;
	default:
		error("Failed to retreive dimension from unknown vertex attribute type %d", type);
		return -1;
	}
}

s32 vertex_component_size(Vertex_Component_Type type) {
	switch (type) {
    case VERTEX_S32:   return 1 * sizeof(s32);
    case VERTEX_U32:   return 1 * sizeof(u32);
	case VERTEX_F32_2: return 2 * sizeof(f32);
	case VERTEX_F32_3: return 3 * sizeof(f32);
	case VERTEX_F32_4: return 4 * sizeof(f32);
	default:
		error("Failed to retreive size from unknown vertex attribute type %d", type);
		return -1;
	}
}
