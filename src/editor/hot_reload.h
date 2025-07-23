#pragma once

#include "asset.h"
#include "os/sync.h"

typedef void *Thread; // from os/thread.h

struct Hot_Reload_List {
    static constexpr u32 MAX_DIRS  = 16;
    static constexpr u32 MAX_COUNT = 8;    
    
	const char *directory_paths[MAX_DIRS];
    sid hot_reload_paths[MAX_COUNT];
	u16 path_count   = 0;
    u16 reload_count = 0;
    Semaphore semaphore = SEMAPHORE_NONE;
};

Thread start_hot_reload_thread(Hot_Reload_List &list);
void register_hot_reload_directory(Hot_Reload_List &list, const char *path);
void check_hot_reload(Hot_Reload_List &list);
