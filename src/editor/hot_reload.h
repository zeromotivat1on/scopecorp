#pragma once

inline constexpr s32 MAX_HOT_RELOAD_LIST_SIZE = 16;

typedef void (*Hot_Reload_Callback)(const char* filename);

struct Hot_Reload_Directory {
    const char* path;
    Hot_Reload_Callback callback;
};

struct Hot_Reload_List {
    Hot_Reload_Directory dirs[MAX_HOT_RELOAD_LIST_SIZE];
    s32 watch_count = 0;
};

void register_hot_reload_dir(Hot_Reload_List* list, const char* path, Hot_Reload_Callback callback);
void start_hot_reload_thread(Hot_Reload_List* list);
void stop_hot_reload_thread(Hot_Reload_List* list);
