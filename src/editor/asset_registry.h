#pragma once

inline constexpr s32 MAX_ASSET_PATH_SIZE = 128;

enum Asset_Type
{
    ASSET_UNKNOWN,
    ASSET_SHADER,
    ASSET_TEXTURE,
    // More to add.
};

struct Asset_Handle
{
    s32 idx = INVALID_INDEX;
    Asset_Type type = ASSET_UNKNOWN;
};

struct Asset_Entry
{
    u64 id; // handle to gfx api data
    u32 version;
    u32 ref_count; // do we actually need this?
    char filepath[MAX_ASSET_PATH_SIZE];
};

struct Asset_Registry
{
    // @Cleanup: not sure about this, maybe its better to store arrays
    // for each asset type separately, like game entities.
    Asset_Entry* entries;
    s32 capacity;
    s32 count;
    s32 free_idx;
    //Critical_Section cs; // for thread safe asset operations
};

Asset_Registry* create_asset_registry(s32 capacity);
bool valid_handle(Asset_Registry* registry, Asset_Handle handle);
Asset_Handle load_asset(Asset_Registry* registry, const char* path, Asset_Type type);
Asset_Entry* find_asset(Asset_Registry* registry, Asset_Handle handle);
