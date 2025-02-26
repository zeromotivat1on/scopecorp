#include "pch.h"
#include "render/render_registry.h"
#include <string.h>

void init_render_registry(Render_Registry* registry)
{
    registry->vertex_buffers = Array<Vertex_Buffer>(MAX_VERTEX_BUFFERS);
    registry->index_buffers  = Array<Index_Buffer>(MAX_INDEX_BUFFERS);
    registry->shaders        = Array<Shader>(MAX_SHADERS);
    registry->textures       = Array<Texture>(MAX_TEXTURES);
}
