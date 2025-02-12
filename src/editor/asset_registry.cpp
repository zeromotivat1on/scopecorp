#include "pch.h"
#include "asset_registry.h"
#include <string.h>

Asset_Registry* create_asset_registry(s32 capacity)
{
    Asset_Registry* registry = alloc_struct_persistent(Asset_Registry);
    registry->entries = alloc_array_persistent(capacity, Asset_Entry);
    registry->capacity = capacity;
    registry->count = 0;
    registry->free_idx = 0;
    return registry;
}

bool valid_handle(Asset_Registry* registry, Asset_Handle handle)
{
    return handle.idx >= 0
        && handle.idx < registry->count
        && handle.idx != registry->free_idx;
    // And maybe extract asset type from extension and check with type in handle.
}

Asset_Handle load_asset(Asset_Registry* registry, const char* path, Asset_Type type)
{
    
}

Asset_Entry* find_asset(Asset_Registry* registry, Asset_Handle handle)
{
    if (handle.idx == registry->free_idx) return null;
    if (handle.idx >= registry->count) return null;

    load_asset(asset_registry, "shaders/player.glsl", ASSET_SHADER);
    load_shader(asset_registry, "shaders/player.glsl");
}
