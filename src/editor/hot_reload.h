#pragma once

inline constexpr s32 MAX_HOT_RELOAD_LIST_SIZE = 16;

// @Cleanup: in case of shader hot reload, 1 or 2 attempts can be done
// to open modified shader file to recreate it, but it can still be blocked
// (maybe by win32 watch dir api), so errors will be logged and we will try
// hot reload on next frame. Thats why this interval is added, more like @Hack.
inline constexpr f32 HOT_RELOAD_CHECK_INTERVAL = 0.1f; // in seconds

typedef u64 sid;
typedef void (*Hot_Reload_Callback)(const char *filename);

struct Hot_Reload_Directory {
	const char *path = null;
	Hot_Reload_Callback callback = null;
};

struct Hot_Reload_List {
	Hot_Reload_Directory dirs[MAX_HOT_RELOAD_LIST_SIZE];
	s32 watch_count = 0;
};

void register_hot_reload_dir(Hot_Reload_List *list, const char *path, Hot_Reload_Callback callback);
void start_hot_reload_thread(Hot_Reload_List *list);
void stop_hot_reload_thread(Hot_Reload_List *list);


inline constexpr s32 MAX_SHADER_HOT_RELOAD_QUEUE_SIZE = 4;

struct Shader_Hot_Reload_Queue {
	sid sids[MAX_SHADER_HOT_RELOAD_QUEUE_SIZE] = {0};
	s32 count = 0;
};

inline Shader_Hot_Reload_Queue shader_hot_reload_queue;

bool hot_reload_shader(sid shader_sid);
void on_shader_changed_externally(const char *path);
void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue *queue, f32 dt);
