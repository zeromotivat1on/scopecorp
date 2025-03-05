#include "pch.h"
#include "editor/hot_reload.h"

#include "log.h"

#include "render/render_registry.h"

// @Cleanup: do not need this as recreate_shader exists now?
bool hot_reload_shader(s32 shader_index) {
	auto &shader = render_registry.shaders[shader_index];
	if (recreate_shader(&shader)) {
		log("Hot reloaded shader %s", shader.path);
		return true;
	}

	error("Failed to hot reload shader %s, see errors above", shader.path);
	return false;
}

void on_shader_changed_externally(const char *path) {
	const s32 shader_index = find_shader_by_file(&shader_index_list, path);
	if (shader_index == INVALID_INDEX) return;

	bool already_in_queue = false;
	for (s32 i = 0; i < shader_hot_reload_queue.count; ++i) {
		if (shader_index == shader_hot_reload_queue.indices[i])
			already_in_queue = true;
	}

	if (!already_in_queue) {
		assert(shader_hot_reload_queue.count < MAX_SHADER_HOT_RELOAD_QUEUE_SIZE);
		shader_hot_reload_queue.indices[shader_hot_reload_queue.count] = shader_index;
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
		const s32 index = queue->indices[i];
		// @Cleanup: not correct in case of success of not last shader in queue,
		// but in theory queue should always have max 1 shader in it.
		if (hot_reload_shader(index)) queue->count--;
	}

}
