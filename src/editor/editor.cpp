#include "pch.h"
#include "editor/hot_reload.h"

#include "os/time.h"
#include "os/file.h"

#include "render/render_registry.h"

#include "log.h"
#include "profile.h"
#include "asset_registry.h"

#include <string.h>

// @Cleanup: do not need this as recreate_shader exists now?
bool hot_reload_shader(sid shader_sid) {
    START_SCOPE_TIMER(time);
    
    const Asset &asset = asset_table[shader_sid];
    
    char path[MAX_PATH_SIZE];
    convert_to_full_asset_path(asset.relative_path, path);

    char *source = push_array(temp, MAX_SHADER_SIZE, char);
    defer { pop(temp, MAX_SHADER_SIZE); };

    u64 bytes_read = 0;
    if (!read_file(path, source, MAX_SHADER_SIZE, &bytes_read)) {
        return false;
    }

    source[bytes_read] = '\0';
    
	if (recreate_shader(asset.registry_index, source)) {
		log("Hot reloaded shader %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(time));
		return true;
	}

	error("Failed to hot reload shader %s, see errors above", path);
    return false;
}

void on_shader_changed_externally(const char *path) {
    char relative_path[MAX_PATH_SIZE];
    strcpy(relative_path, path);
    convert_to_relative_asset_path(relative_path);

    const auto shader_sid = cache_sid(relative_path);
    if (!asset_table.find(shader_sid)) {
        error("Failed to find asset for shader %s", path);
        return;
    }

	bool already_in_queue = false;
	for (s32 i = 0; i < shader_hot_reload_queue.count; ++i) {
		if (shader_sid == shader_hot_reload_queue.sids[i])
			already_in_queue = true;
	}

	if (!already_in_queue) {
		assert(shader_hot_reload_queue.count < MAX_SHADER_HOT_RELOAD_QUEUE_SIZE);
		shader_hot_reload_queue.sids[shader_hot_reload_queue.count] = shader_sid;
		shader_hot_reload_queue.count++;
	}
}

void check_shader_hot_reload_queue(Shader_Hot_Reload_Queue *queue, f32 dt) {
    static f32 time_since_last_interval = 0.0f;
    time_since_last_interval += dt;

    if (time_since_last_interval < HOT_RELOAD_CHECK_INTERVAL) return;
    time_since_last_interval = 0.0f;

	if (queue->count == 0) return;

	for (s32 i = 0; i < queue->count; ++i) {
		// @Cleanup: not correct in case of success of not last shader in queue,
		// but in theory queue should always have max 1 shader in it.
		if (hot_reload_shader(queue->sids[i])) queue->count--;
	}
}

