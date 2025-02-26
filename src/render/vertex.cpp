#include "pch.h"
#include "vertex.h"
#include "log.h"

s32 vertex_attrib_dimension(Vertex_Attrib_Type type) {
    switch(type) {
    case VERTEX_ATTRIB_NULL:   return 0;
    case VERTEX_ATTRIB_F32_V2: return 2;
    case VERTEX_ATTRIB_F32_V3: return 3;
    default:
        error("Failed to retreive dimension from unknown vertex attribute type %d", type);
        return -1;
    }
}

s32 vertex_attrib_size(Vertex_Attrib_Type type) {
    switch(type) {
    case VERTEX_ATTRIB_NULL:   return 0;
    case VERTEX_ATTRIB_F32_V2: return 2 * sizeof(f32);
    case VERTEX_ATTRIB_F32_V3: return 3 * sizeof(f32);
    default:
        error("Failed to retreive size from unknown vertex attribute type %d", type);
        return -1;
    }    
}
