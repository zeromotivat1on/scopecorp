#include "pch.h"
#include "profile.h"
#include "str.h"
#include "log.h"
#include "font.h"
#include "hash.h"
#include "input_stack.h"
#include "stb_sprintf.h"

#include "os/time.h"
#include "os/input.h"
#include "os/window.h"

#include "editor/editor.h"

#include "game/game.h"
#include "game/world.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/render_stats.h"
#include "render/buffer_storage.h"

Input_Key KEY_SWITCH_RUNTIME_PROFILER = KEY_F5;
Input_Key KEY_SWITCH_MEMORY_PROFILER  = KEY_F6;
 
Scope_Timer::Scope_Timer(const char *info)
    : info(info), start(os_perf_counter()) {}

Scope_Timer::~Scope_Timer() {
    const f32 ms = (os_perf_counter() - start) / (f32)os_perf_frequency_ms();
    log("%s %.2fms", info, ms);
}

Profile_Scope::Profile_Scope(const char *scope_name, const char *scope_file_path, u32 scope_line) {
    Assert(str_size(scope_name) < MAX_PROFILE_SCOPE_NAME_SIZE);
    
    name      = scope_name;
    file_path = scope_file_path;
    line      = scope_line;

    start = os_perf_counter();
}

Profile_Scope::~Profile_Scope() {
    end = os_perf_counter();
    diff = end - start;

    auto &rp = runtime_profiler;
    
    Profile_Scope *scope = null;
    for (u32 i = 0; i < rp.scope_count; ++i) {
        auto &ps = rp.scopes[i];
        if (str_cmp(name, ps.name)) {
            scope = &ps;
        }
    }

    if (!scope) {
        Assert(rp.scope_count < MAX_PROFILER_SCOPES);
        scope = rp.scopes + rp.scope_count;
        rp.scope_count += 1;
    }

    *scope = *this;
}

void init_runtime_profiler() {
    auto &rp = runtime_profiler;
    rp.scopes      = allocltn(Profile_Scope, MAX_PROFILER_SCOPES);
    rp.scope_times = allocltn(f32,           MAX_PROFILER_SCOPES);
}

void open_runtime_profiler() {
    auto &rp = runtime_profiler;
    Assert(!rp.is_open);
    
    rp.is_open = true;

    push_input_layer(&input_layer_runtime_profiler);
}

void close_runtime_profiler() {
    auto &rp = runtime_profiler;
    Assert(rp.is_open);

    rp.is_open = false;

    pop_input_layer();
}

void on_input_runtime_profiler(Window_Event *event) {
    const bool press = event->key_press;
    const auto key = event->key_code;
        
    switch (event->type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_window_close(window);
        } else if (press && key == KEY_SWITCH_RUNTIME_PROFILER) {
            close_runtime_profiler();
        }
        
        break;
    }
    }
}

void draw_runtime_profiler() {
    PROFILE_SCOPE(__FUNCTION__);

    auto &rp = runtime_profiler;
    rp.scope_time_update_time += delta_time;

    if (rp.scope_time_update_time > rp.scope_time_update_interval) {
        for (u32 i = 0; i < rp.scope_count; ++i) {
            const auto &scope = rp.scopes[i];
            rp.scope_times[i] = scope.diff / (f32)os_perf_frequency_ms();
        }

        rp.scope_time_update_time = 0.0f;
    }

    if (!rp.is_open) return;
    
    constexpr f32 PROFILER_MARGIN  = 100.0f;
    constexpr f32 PROFILER_PADDING = 16.0f;

    const auto &atlas = ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;

    {   // Profiler quad.
        const vec2 q0 = vec2(PROFILER_MARGIN,
                             viewport.height - PROFILER_MARGIN - 2 * PROFILER_PADDING - rp.scope_count * atlas.line_height);
        const vec2 q1 = vec2(viewport.width - PROFILER_MARGIN,
                             viewport.height - PROFILER_MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(q0, q1, color);
    }

    {   // Profiler scopes.
        vec2 pos = vec2(PROFILER_MARGIN + PROFILER_PADDING, viewport.height - PROFILER_MARGIN - PROFILER_PADDING - ascent);
        
        const s32 space_width_px = get_char_width_px(&atlas, ASCII_SPACE);
        const f32 column_offset_1 = PROFILER_MARGIN + PROFILER_PADDING;
        const f32 column_offset_2 = column_offset_1 + space_width_px * MAX_PROFILE_SCOPE_NAME_SIZE + 1;
        const f32 column_offset_3 = column_offset_2 + space_width_px * MAX_PROFILE_SCOPE_NAME_SIZE * 0.5f + 1;
        
        char buffer[256];
        s32 count = 0;

        // @Todo: sort by time, name etc.
        for (u32 i = 0; i < rp.scope_count; ++i) {
            const auto &scope = rp.scopes[i];
            const f32 time = rp.scope_times[i];

            const f32 max_time = 0.6f;
            const f32 time_alpha = Clamp(time / max_time, 0.0f, 1.0f);
            const u8 r = (u8)(255 * time_alpha);
            const u8 g = (u8)(255 * (1.0f - time_alpha));
            
            const u32 color = rgba_pack(r, g, 0, 255);
                
            pos.x = column_offset_1;
            count = stbsp_snprintf(buffer, sizeof(buffer), "%s", scope.name);
            ui_text(buffer, count, pos, color, UI_PROFILER_FONT_ATLAS_INDEX);

            pos.x = column_offset_2;
            count = stbsp_snprintf(buffer, sizeof(buffer), "%.2fms", time);
            ui_text(buffer, count, pos, color, UI_PROFILER_FONT_ATLAS_INDEX);

            pos.x = column_offset_3;
            count = stbsp_snprintf(buffer, sizeof(buffer), "%s:%d",
                                   scope.file_path, scope.line);
            ui_text(buffer, count, pos, color, UI_PROFILER_FONT_ATLAS_INDEX);
            
            pos.y -= atlas.line_height;
        }
    }

    rp.scope_count = 0;
}

void init_memory_profiler() {
    auto &mp = memory_profiler;
}

void open_memory_profiler() {
    auto &mp = memory_profiler;
    Assert(!mp.is_open);
    
    mp.is_open = true;

    push_input_layer(&input_layer_memory_profiler);
}

void close_memory_profiler() {
    auto &mp = memory_profiler;
    Assert(mp.is_open);

    mp.is_open = false;

    pop_input_layer();
}

void draw_memory_profiler() {
    auto &mp = memory_profiler;

    if (!mp.is_open) return;
    
    constexpr f32 PROFILER_MARGIN  = 100.0f;
    constexpr f32 PROFILER_PADDING = 16.0f;
    constexpr u8  PROFILER_MAX_LINE_COUNT = 4;

    const auto &atlas = ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;

    {   // Profiler quad.
        const vec2 q0 = vec2(PROFILER_MARGIN,
                             viewport.height - PROFILER_MARGIN - 2 * PROFILER_PADDING - PROFILER_MAX_LINE_COUNT * atlas.line_height);
        const vec2 q1 = vec2(viewport.width - PROFILER_MARGIN,
                             viewport.height - PROFILER_MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(q0, q1, color);
    }

    {   // Profiler scopes.
        extern u64 allocl_size, allocf_size;

        vec2 pos = vec2(PROFILER_MARGIN + PROFILER_PADDING, viewport.height - PROFILER_MARGIN - PROFILER_PADDING - ascent);
        const u32 color = rgba_white;
    
        char buffer[256];
        s32 count = 0;

        const f32 allocl_percent = (f32)allocl_size / MAX_ALLOCL_SIZE * 100.0f;
		count = (s32)stbsp_snprintf(buffer, sizeof(buffer),
                                    "CPU Linear %.2fmb/%.2fmb (%.2f%%)",
                                    (f32)allocl_size / 1024 / 1024, (f32)MAX_ALLOCL_SIZE / 1024 / 1024, allocl_percent);
		ui_text(buffer, count, pos, rgba_white);
        pos.y -= atlas.line_height;

        const f32 allocf_percent = (f32)allocf_size / MAX_ALLOCF_SIZE * 100.0f;
		count = (s32)stbsp_snprintf(buffer, sizeof(buffer),
                                    "CPU Frame  %.2fmb/%.2fmb (%.2f%%)",
                                    (f32)allocf_size / 1024 / 1024, (f32)MAX_ALLOCF_SIZE / 1024 / 1024, allocf_percent);
		ui_text(buffer, count, pos, rgba_white);
        pos.y -= atlas.line_height;

        const auto &vbs = vertex_buffer_storage;
        const f32 rallocv_percent = (f32)vbs.size / vbs.capacity * 100.0f;
		count = (s32)stbsp_snprintf(buffer, sizeof(buffer),
                                    "GPU Vertex %.2fmb/%.2fmb (%.2f%%)",
                                    (f32)vbs.size / 1024 / 1024, (f32)vbs.capacity / 1024 / 1024, rallocv_percent);
		ui_text(buffer, count, pos, rgba_white);
        pos.y -= atlas.line_height;

        const auto &ibs = index_buffer_storage;
        const f32 ralloci_percent = (f32)ibs.size / ibs.capacity * 100.0f;
		count = (s32)stbsp_snprintf(buffer, sizeof(buffer),
                                    "GPU Index  %.2fmb/%.2fmb (%.2f%%)",
                                    (f32)ibs.size / 1024 / 1024, (f32)ibs.capacity / 1024 / 1024, ralloci_percent);
		ui_text(buffer, count, pos, rgba_white);
        pos.y -= atlas.line_height;
    }
}

void on_input_memory_profiler(Window_Event *event) {
    const bool press = event->key_press;
    const auto key = event->key_code;
        
    switch (event->type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_window_close(window);
        } else if (press && key == KEY_SWITCH_MEMORY_PROFILER) {
            close_memory_profiler();
        }
        
        break;
    }
    }
}

void draw_dev_stats() {
    PROFILE_SCOPE(__FUNCTION__);
    
    const auto &atlas = ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX];
	const auto &player = world->player;
	const auto &camera = *desired_camera(world);

	static char text[256];
	const f32 padding = atlas.font_size * 0.5f;
	const vec2 shadow_offset = vec2(atlas.font_size * 0.1f, -atlas.font_size * 0.1f);
	s32 text_size = 0;
	vec2 pos;

	{   // Entity.
        pos.x = padding;
		pos.y = (f32)viewport.height - atlas.line_height;
                
        if (editor.mouse_picked_entity) {
            const auto *e = editor.mouse_picked_entity;
            const auto property_to_change = game_state.selected_entity_property_to_change;
            
            text_size = (s32)stbsp_snprintf(text, sizeof(text), "%s %u\n%slocation %s\n%srotation %s\n%sscale %s",
                                            to_string(e->type), e->eid,
                                            property_to_change == PROPERTY_LOCATION ? " -> " : "\t", to_string(e->location),
                                            property_to_change == PROPERTY_ROTATION ? " -> " : "\t", to_string(e->rotation),
                                            property_to_change == PROPERTY_SCALE ? " -> " : "\t", to_string(e->scale));
            ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
            pos.y -= 4 * atlas.line_height;
        }
	}

	{   // Stats & states.
        pos.y = (f32)viewport.height - atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text),
                                        "%s %s",
                                        GAME_VERSION, build_type_name);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
		pos.y -= atlas.line_height;

		text_size = (s32)stbsp_snprintf(text, sizeof(text),
                                        "%.2fms %.ffps",
                                        average_dt * 1000.0f, average_fps);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
		pos.y -= atlas.line_height;
        
        text_size = (s32)stbsp_snprintf(text, sizeof(text),
                                        "window %dx%d",
                                        window->width, window->height);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
		pos.y -= atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text),
                                        "viewport %dx%d",
                                        viewport.width, viewport.height);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
        pos.y -= atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text),
                                        "draw calls %d",
                                        draw_call_count);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_text_with_shadow(text, text_size, pos, rgba_white, shadow_offset, rgba_black);
		pos.y -= atlas.line_height;
	}

    draw_call_count = 0;
}
