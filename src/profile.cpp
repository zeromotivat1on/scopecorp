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

#include "game/game.h"
#include "game/world.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/render_stats.h"

s16 KEY_SWITCH_PROFILER = KEY_F9;
 
Scope_Timer::Scope_Timer(const char *info)
    : info(info), start(performance_counter()) {}

Scope_Timer::~Scope_Timer() {
    const f32 ms = (performance_counter() - start) / (f32)performance_frequency_ms();
    log("%s %.2fms", info, ms);
}

Profile_Scope::Profile_Scope(const char *scope_name, const char *scope_filepath, u32 scope_line) {
    name     = scope_name;
    filepath = scope_filepath;
    line     = scope_line;

    start = performance_counter();
}

Profile_Scope::~Profile_Scope() {
    end = performance_counter();
    diff = end - start;

    Profile_Scope *scope = null;
    for (u32 i = 0; i < profiler.scope_count; ++i) {
        auto &ps = profiler.scopes[i];
        if (str_cmp(name, ps.name)) {
            scope = &ps;
        }
    }

    if (!scope) {
        Assert(profiler.scope_count < MAX_PROFILER_SCOPES);
        scope = profiler.scopes + profiler.scope_count;
        profiler.scope_count += 1;
    }

    *scope = *this;
}

void init_profiler() {
    profiler.scopes      = allocltn(Profile_Scope, MAX_PROFILER_SCOPES);
    profiler.scope_times = allocltn(f32,           MAX_PROFILER_SCOPES);
    profiler.scope_count = 0;
    profiler.is_open = false;
}

void open_profiler() {
    Assert(!profiler.is_open);
    
    profiler.is_open = true;

    push_input_layer(&input_layer_profiler);
}

void close_profiler() {
    Assert(profiler.is_open);

    profiler.is_open = false;

    pop_input_layer();
}

void on_input_profiler(Window_Event *event) {
    switch (event->type) {
    case WINDOW_EVENT_KEYBOARD: {
        const bool press = event->key_press;
        const auto key = event->key_code;

        if (press && key == KEY_SWITCH_PROFILER) {
            close_profiler();
        }
        
        break;
    }
    }
}

void draw_profiler() {
    profiler.scope_time_update_time += delta_time;

    if (profiler.scope_time_update_time > profiler.scope_time_update_interval) {
        for (u32 i = 0; i < profiler.scope_count; ++i) {
            const auto &scope = profiler.scopes[i];
            profiler.scope_times[i] = scope.diff / (f32)performance_frequency_ms();
        }

        profiler.scope_time_update_time = 0.0f;
    }

    if (!profiler.is_open) return;
    
    constexpr f32 PROFILER_MARGIN  = 100.0f;
    constexpr f32 PROFILER_PADDING = 16.0f;

    const auto &atlas = *ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;

    {   // Profiler quad.
        const vec2 q0 = vec2(PROFILER_MARGIN);
        const vec2 q1 = vec2(viewport.width - PROFILER_MARGIN, viewport.height - PROFILER_MARGIN);
        const vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.8f);
        ui_draw_quad(q0, q1, color);
    }

    {   // Profiler scopes.
        vec2 pos = vec2(PROFILER_MARGIN + PROFILER_PADDING, viewport.height - PROFILER_MARGIN - PROFILER_PADDING - ascent);
        const vec4 color = vec4_white;
    
        char buffer[256];
        s32 count = 0;

        const auto &atlas = *ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX];

        // @Todo: sort by time, name etc.
        for (u32 i = 0; i < profiler.scope_count; ++i) {
            const auto &scope = profiler.scopes[i];
            const f32 time = profiler.scope_times[i];
        
            count = stbsp_snprintf(buffer, sizeof(buffer), "%s %s:%u %.2fms",
                                   scope.name, scope.filepath, scope.line, time);
            ui_draw_text(buffer, count, pos, color, UI_PROFILER_FONT_ATLAS_INDEX);
            pos.y -= atlas.line_height;
        }
    }

    profiler.scope_count = 0;
}

void draw_dev_stats() {
    PROFILE_SCOPE(__FUNCTION__);
    
    const auto &atlas = *ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX];
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
        
        const auto &player_aabb = world->aabbs[player.aabb_index];
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "player\n\tlocation %s\n\tvelocity %s\n\taabb %s %s", to_string(player.location), to_string(player.velocity), to_string(player_aabb.min), to_string(player_aabb.max));
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
		pos.y -= 4 * atlas.line_height;

		text_size = (s32)stbsp_snprintf(text, sizeof(text), "camera\n\teye %s\n\tat %s", to_string(camera.eye), to_string(camera.at));
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
		pos.y -= 3 * atlas.line_height;
        
        if (world->mouse_picked_entity) {
            const auto *e = world->mouse_picked_entity;
            const auto property_to_change = game_state.selected_entity_property_to_change;
            
            text_size = (s32)stbsp_snprintf(text, sizeof(text), "%s %d\n%slocation %s\n%srotation %s\n%sscale %s",
                                            to_string(e->type), e->id,
                                            property_to_change == PROPERTY_LOCATION ? " -> " : "\t", to_string(e->location),
                                            property_to_change == PROPERTY_ROTATION ? " -> " : "\t", to_string(e->rotation),
                                            property_to_change == PROPERTY_SCALE ? " -> " : "\t", to_string(e->scale));
            ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
            pos.y -= 4 * atlas.line_height;
        }
	}

	{   // Stats & states.
        extern u64 allocl_size, allocf_size;

        pos.y = (f32)viewport.height - atlas.line_height;
        
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "%.2fms %.ffps %s %s", average_dt * 1000.0f, average_fps, build_type_name, to_string(game_state.mode));
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
		pos.y -= atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text), "window %dx%d", window->width, window->height);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
		pos.y -= atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text), "viewport %dx%d", viewport.width, viewport.height);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
        pos.y -= atlas.line_height;

        text_size = (s32)stbsp_snprintf(text, sizeof(text), "draw calls %d", draw_call_count);
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
		pos.y -= atlas.line_height;
        
        text_size = (s32)stbsp_snprintf(text, sizeof(text), "%s %s", to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
		pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
        pos.y -= atlas.line_height;

        const f32 allocl_percent = (f32)allocl_size / MAX_ALLOCL_SIZE * 100.0f;
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "Linear %.2fmb/%.2fmb (%.2f%%)", (f32)allocl_size / 1024 / 1024, (f32)MAX_ALLOCL_SIZE / 1024 / 1024, allocl_percent);
        pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
        pos.y -= atlas.line_height;

        const f32 allocf_percent = (f32)allocf_size / MAX_ALLOCF_SIZE * 100.0f;
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "Frame %.2fmb/%.2fmb (%.2f%%)", (f32)allocf_size / 1024 / 1024, (f32)MAX_ALLOCF_SIZE / 1024 / 1024, allocf_percent);
        pos.x = viewport.width - get_line_width_px(&atlas, text, text_size) - padding;
		ui_draw_text_with_shadow(text, text_size, pos, vec4_white, shadow_offset, vec4_black);
        pos.y -= atlas.line_height;
	}

    draw_call_count = 0;
}
