#pragma once

inline constexpr s32 MAX_HOT_RELOAD_DIRECTORIES = 16;
inline constexpr s32 MAX_HOT_RELOAD_ASSETS      = 8;

typedef u64 sid;

struct Hot_Reload_List {
	const char *paths[MAX_HOT_RELOAD_DIRECTORIES];
    sid reload_sids[MAX_HOT_RELOAD_ASSETS];
	s32 path_count   = 0;
    s32 reload_count = 0;
};

void register_hot_reload_directory(Hot_Reload_List *list, const char *path);
void check_for_hot_reload(Hot_Reload_List *list);
