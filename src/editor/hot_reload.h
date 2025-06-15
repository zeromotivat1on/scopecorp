#pragma once

#include "asset.h"
#include "os/sync.h"
#include "os/thread.h"

inline constexpr s32 MAX_HOT_RELOAD_DIRECTORIES = 16;
inline constexpr s32 MAX_HOT_RELOAD_ASSETS      = 8;

struct Hot_Reload_Asset {
    Asset_Type asset_type = ASSET_NONE;
    sid sid = SID_NONE;
};

struct Hot_Reload_List {
	const char *directory_paths[MAX_HOT_RELOAD_DIRECTORIES];
    Hot_Reload_Asset hot_reload_assets[MAX_HOT_RELOAD_ASSETS];
	s32 path_count   = 0;
    s32 reload_count = 0;
    Semaphore semaphore = SEMAPHORE_NONE;
};

Thread start_hot_reload_thread(Hot_Reload_List *list);
void register_hot_reload_directory(Hot_Reload_List *list, const char *path);
void check_for_hot_reload(Hot_Reload_List *list);
