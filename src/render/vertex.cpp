#include "pch.h"
#include "vertex.h"

s32 vertex_attrib_dimension(Vertex_Attrib_Type type)
{
    switch(type)
    {
        case VERTEX_ATTRIB_NULL: return 0;
        case VERTEX_ATTRIB_F32_V2: return 2;
        case VERTEX_ATTRIB_F32_V3: return 3;
    }
}

s32 vertex_attrib_size(Vertex_Attrib_Type type)
{
    switch(type)
    {
        case VERTEX_ATTRIB_NULL: return 0;
        case VERTEX_ATTRIB_F32_V2: return 2 * sizeof(f32);
        case VERTEX_ATTRIB_F32_V3: return 3 * sizeof(f32);
    }    
}
