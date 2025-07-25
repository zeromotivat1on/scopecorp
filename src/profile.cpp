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
#include "render/r_viewport.h"
#include "render/r_stats.h"
#include "render/r_storage.h"

Input_Key KEY_SWITCH_RUNTIME_PROFILER = KEY_F5;
Input_Key KEY_SWITCH_MEMORY_PROFILER  = KEY_F6;
 
Scope_Timer::Scope_Timer(const char *info)
    : info(info), start(os_perf_counter()) {}

Scope_Timer::~Scope_Timer() {
    const f32 ms = (os_perf_counter() - start) / (f32)os_perf_hz_ms();
    log("%s %.2fms", info, ms);
}

Profile_Scope::Profile_Scope(const char *scope_name, const char *scope_file_path, u32 scope_line) {
    Assert(str_size(scope_name) < Runtime_profiler.MAX_NAME_LENGTH);
    
    name      = scope_name;
    file_path = scope_file_path;
    line      = scope_line;

    start = os_perf_counter();
}

Profile_Scope::~Profile_Scope() {
    end = os_perf_counter();
    diff = end - start;

    auto &rp = Runtime_profiler;
    
    Profile_Scope *scope = null;
    for (u32 i = 0; i < rp.scope_count; ++i) {
        auto &ps = rp.scopes[i];
        if (str_cmp(name, ps.name)) {
            scope = &ps;
        }
    }

    if (!scope) {
        Assert(rp.scope_count < rp.MAX_SCOPES);
        scope = rp.scopes + rp.scope_count;
        rp.scope_count += 1;
    }

    *scope = *this;
}

void init_runtime_profiler() {
    auto &rp = Runtime_profiler;
    rp.scopes      = allocpn(Profile_Scope, rp.MAX_SCOPES);
    rp.scope_times = allocpn(f32,           rp.MAX_SCOPES);
}

void open_runtime_profiler() {
    auto &rp = Runtime_profiler;
    Assert(!rp.is_open);
    
    rp.is_open = true;

    push_input_layer(Input_layer_runtime_profiler);
}

void close_runtime_profiler() {
    auto &rp = Runtime_profiler;
    Assert(rp.is_open);

    rp.is_open = false;

    pop_input_layer();
}

void on_input_runtime_profiler(const Window_Event &event) {
    const bool press = event.key_press;
    const auto key = event.key_code;
        
    switch (event.type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_close_window(window);
        } else if (press && key == KEY_SWITCH_RUNTIME_PROFILER) {
            close_runtime_profiler();
        }
        
        break;
    }
    }
}

void draw_runtime_profiler() {
    PROFILE_SCOPE(__FUNCTION__);

    auto &rp = Runtime_profiler;
    rp.scope_time_update_time += delta_time;

    if (rp.scope_time_update_time > rp.scope_time_update_interval) {
        for (u32 i = 0; i < rp.scope_count; ++i) {
            const auto &scope = rp.scopes[i];
            rp.scope_times[i] = scope.diff / (f32)os_perf_hz_ms();
        }

        rp.scope_time_update_time = 0.0f;
    }

    if (!rp.is_open) return;
    
    constexpr f32 MARGIN  = 100.0f;
    constexpr f32 PADDING = 16.0f;
    constexpr f32 QUAD_Z = 0.0f;
    
    const auto &atlas = R_ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;

    {   // Profiler quad.
        const vec2 p0 = vec2(MARGIN,
                             R_viewport.height - MARGIN - 2 * PADDING - rp.scope_count * atlas.line_height);
        const vec2 p1 = vec2(R_viewport.width - MARGIN,
                             R_viewport.height - MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, color, QUAD_Z);
    }

    {   // Profiler scopes.
        vec2 pos = vec2(MARGIN + PADDING, R_viewport.height - MARGIN - PADDING - ascent);
        
        const s32 space_width_px = get_char_width_px(atlas, ASCII_SPACE);
        const f32 column_offset_1 = MARGIN + PADDING;
        const f32 column_offset_2 = column_offset_1 + space_width_px * rp.MAX_NAME_LENGTH + 1;
        const f32 column_offset_3 = column_offset_2 + space_width_px * rp.MAX_NAME_LENGTH * 0.5f + 1;
        
        char buffer[256];
        s32 count = 0;

        const auto atlas_index = UI_PROFILER_FONT_ATLAS_INDEX;
        
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
            ui_text(buffer, count, pos, color, atlas_index);

            pos.x = column_offset_2;
            count = stbsp_snprintf(buffer, sizeof(buffer), "%.2fms", time);
            ui_text(buffer, count, pos, color, atlas_index);

            pos.x = column_offset_3;
            count = stbsp_snprintf(buffer, sizeof(buffer), "%s:%d",
                                   scope.file_path, scope.line);
            ui_text(buffer, count, pos, color, QUAD_Z + F32_EPSILON, atlas_index);
            
            pos.y -= atlas.line_height;
        }
    }

    rp.scope_count = 0;
}

void init_memory_profiler() {
    auto &mp = Memory_profiler;
}

void open_memory_profiler() {
    auto &mp = Memory_profiler;
    Assert(!mp.is_open);
    
    mp.is_open = true;

    push_input_layer(Input_layer_memory_profiler);
}

void close_memory_profiler() {
    auto &mp = Memory_profiler;
    Assert(mp.is_open);

    mp.is_open = false;

    pop_input_layer();
}

void draw_memory_profiler() {
    auto &mp = Memory_profiler;

    if (!mp.is_open) return;
    
    constexpr f32 MARGIN  = 100.0f;
    constexpr f32 PADDING = 16.0f;
    constexpr f32 QUAD_Z = 0.0f;
    constexpr u32 MAX_LINE_COUNT = 5;

    const auto &atlas = R_ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;

    {   // Profiler quad.
        const vec2 p0 = vec2(MARGIN,
                             R_viewport.height - MARGIN - 2 * PADDING - MAX_LINE_COUNT * atlas.line_height);
        const vec2 p1 = vec2(R_viewport.width - MARGIN,
                             R_viewport.height - MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, color, QUAD_Z);
    }

    {   // Profiler scopes.
        struct Mem_Scope {
            const char *name = null;
            u64 size = 0;
            u64 capacity = 0;
        };
        
        extern u64 allocp_size, alloct_size, allocf_size;

        const Mem_Scope scopes[MAX_LINE_COUNT] = {
            { "CPU Persistent", allocp_size, MAX_ALLOCP_SIZE },
            { "CPU Temp",       alloct_size, MAX_ALLOCT_SIZE },
            { "CPU Frame",      allocf_size, MAX_ALLOCF_SIZE },
            { "GPU Vertex",     R_vertex_map_range.size, R_vertex_map_range.capacity },
            { "GPU Index",      R_index_map_range.size,  R_index_map_range.capacity },
        };

        constexpr u32 MAX_NAME_LENGTH = 16;
        constexpr u32 MAX_USAGE_LENGTH = 32;
        
        const u32 space_width_px = get_char_width_px(atlas, ASCII_SPACE);
        const f32 column_offset_1 = MARGIN + PADDING;
        const f32 column_offset_2 = column_offset_1 + space_width_px * MAX_NAME_LENGTH + 1;
        const f32 column_offset_3 = column_offset_2 + space_width_px * MAX_USAGE_LENGTH * 0.5f + 1;

        const auto atlas_index = UI_PROFILER_FONT_ATLAS_INDEX;
        
        vec2 pos = vec2(MARGIN + PADDING, R_viewport.height - MARGIN - PADDING - ascent);
        for (u32 i = 0; i < MAX_LINE_COUNT; ++i) {
            const auto &scope = scopes[i];

            const f32 alpha = Clamp((f32)scope.size / scope.capacity, 0.0f, 1.0f);
            const u8 r = (u8)(255 * alpha);
            const u8 g = (u8)(255 * (1.0f - alpha));
            
            const u32 color = rgba_pack(r, g, 0, 255);
            
            char buffer[256];
            u32 count = 0;

            pos.x = column_offset_1;
            count = stbsp_snprintf(buffer, sizeof(buffer),
                                   "%s", scope.name);
            ui_text(buffer, count, pos, color, QUAD_Z + F32_EPSILON, atlas_index);

            pos.x = column_offset_2;
            count = stbsp_snprintf(buffer, sizeof(buffer),
                                   "%.2fmb/%.2fmb",
                                   TO_MB((f32)scope.size),
                                   TO_MB((f32)scope.capacity));
            ui_text(buffer, count, pos, color, QUAD_Z + F32_EPSILON, atlas_index);

            const f32 percent = (f32)scope.size / scope.capacity * 100.f;
            pos.x = column_offset_3;
            count = stbsp_snprintf(buffer, sizeof(buffer),
                                   "%.2f%%", percent);
            ui_text(buffer, count, pos, color, QUAD_Z + F32_EPSILON, atlas_index);

            pos.y -= atlas.line_height;
        }
    }
}

void on_input_memory_profiler(const Window_Event &event) {
    const bool press = event.key_press;
    const auto key = event.key_code;
        
    switch (event.type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_close_window(window);
        } else if (press && key == KEY_SWITCH_MEMORY_PROFILER) {
            close_memory_profiler();
        }
        
        break;
    }
    }
}

void draw_dev_stats() {
    PROFILE_SCOPE(__FUNCTION__);

    constexpr f32 Z = 0.0f;
    
    const auto &atlas = R_ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX];
	const auto &player = World.player;
	const auto &camera = active_camera(World);

	static char text[256];
	const f32 padding = atlas.font_size * 0.5f;
	const vec2 shadow_offset = vec2(atlas.font_size * 0.1f, -atlas.font_size * 0.1f);
	u32 count = 0;

	vec2 pos;
    
	{   // Entity.
        pos.x = padding;
		pos.y = (f32)R_viewport.height - atlas.line_height;
                
        if (Editor.mouse_picked_entity) {
            const auto *e = Editor.mouse_picked_entity;
            const auto property_to_change = game_state.selected_entity_property_to_change;

            const char *change_prop_name = null;
            switch (property_to_change) {
            case PROPERTY_LOCATION: change_prop_name = "location"; break;
            case PROPERTY_ROTATION: change_prop_name = "rotation"; break;
            case PROPERTY_SCALE:    change_prop_name = "scale"; break;
            }
            
            count = stbsp_snprintf(text, sizeof(text), "%s %u %s",
                                   to_string(e->type), e->eid, change_prop_name);
            ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
            pos.y -= 4 * atlas.line_height;
        }
	}

	{   // Stats & states.
        pos.y = (f32)R_viewport.height - atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text),
                               "%s %s",
                               GAME_VERSION, Build_type_name);
		pos.x = R_viewport.width - get_line_width_px(atlas, text, count) - padding;
		ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;

		count = stbsp_snprintf(text, sizeof(text),
                               "%.2fms %.ffps",
                               average_dt * 1000.0f, average_fps);
		pos.x = R_viewport.width - get_line_width_px(atlas, text, count) - padding;
		ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;
        
        count = stbsp_snprintf(text, sizeof(text),
                               "window %dx%d",
                               window.width, window.height);
		pos.x = R_viewport.width - get_line_width_px(atlas, text, count) - padding;
		ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text),
                               "viewport %dx%d",
                               R_viewport.width, R_viewport.height);
		pos.x = R_viewport.width - get_line_width_px(atlas, text, count) - padding;
		ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
        pos.y -= atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text),
                               "draw calls %d",
                               draw_call_count);
		pos.x = R_viewport.width - get_line_width_px(atlas, text, count) - padding;
		ui_text_with_shadow(text, count, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;
	}

    draw_call_count = 0;
}
