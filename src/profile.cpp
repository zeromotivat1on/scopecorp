#include "pch.h"
#include "profile.h"
#include "log.h"
#include "font.h"
#include "memory_storage.h"
#include "stb_sprintf.h"

#include "os/time.h"
#include "os/input.h"
#include "os/window.h"

#include "game/game.h"
#include "game/world.h"

#include "render/text.h"
#include "render/viewport.h"
#include "render/render_stats.h"
#include "render/render_registry.h"

#if !RELEASE
Scope_Timer::Scope_Timer(const char *info)
    : info(info), start(performance_counter()) {}

Scope_Timer::~Scope_Timer() {
    const f32 ms = (performance_counter() - start) / (f32)performance_frequency_ms();
    log("%s %.2fms", info, ms);
}
#endif

void draw_dev_stats() {
    PROFILE_SCOPE(__FUNCTION__);
    
    const auto *atlas = text_draw_buffer.atlas;
	const Player &player = world->player;
	const Camera &camera = *desired_camera(world);

	static char text[256];
	const f32 padding = atlas->font_size * 0.5f;
	const vec2 shadow_offset = vec2(atlas->font_size * 0.1f, -atlas->font_size * 0.1f);
	s32 text_size = 0;
	vec2 pos;

	{   // Entity data.
        pos.x = padding;
		pos.y = (f32)viewport.height - atlas->line_height;
        
        const auto &player_aabb = world->aabbs[player.aabb_index];
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "player\n\tlocation %s\n\tvelocity %s\n\taabb %s %s", to_string(player.location), to_string(player.velocity), to_string(player_aabb.min), to_string(player_aabb.max));
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
		pos.y -= 4 * atlas->line_height;

		text_size = (s32)stbsp_snprintf(text, sizeof(text), "camera\n\teye %s\n\tat %s", to_string(camera.eye), to_string(camera.at));
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
		pos.y -= 3 * atlas->line_height;
        
        if (world->mouse_picked_entity) {
            const auto *e = world->mouse_picked_entity;
            const auto property_to_change = game_state.selected_entity_property_to_change;
            text_size = (s32)stbsp_snprintf(text, sizeof(text), "selected entity\n%slocation %s\n%srotation %s\n%sscale %s",
                                            property_to_change == PROPERTY_LOCATION ? " -> " : "\t", to_string(e->location),
                                            property_to_change == PROPERTY_ROTATION ? " -> " : "\t", to_string(e->rotation),
                                            property_to_change == PROPERTY_SCALE ? " -> " : "\t", to_string(e->scale));
            draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
            pos.y -= 4 * atlas->line_height;
        }
	}

	{   // Runtime stats.
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "%.2fms %.ffps %s %s", average_dt * 1000.0f, average_fps, build_type_name, to_string(game_state.mode));
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y = (f32)viewport.height - atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);

        text_size = (s32)stbsp_snprintf(text, sizeof(text), "window %dx%d", window->width, window->height);
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y -= atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);

        text_size = (s32)stbsp_snprintf(text, sizeof(text), "viewport %dx%d", viewport.width, viewport.height);
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y -= atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);

        const auto &viewport_frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];
        text_size = (s32)stbsp_snprintf(text, sizeof(text), "framebuffer %dx%d", viewport_frame_buffer.width, viewport_frame_buffer.height);
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y -= atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
        
        text_size = (s32)stbsp_snprintf(text, sizeof(text), "draw calls %d", draw_call_count);
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y -= atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
        
        text_size = (s32)stbsp_snprintf(text, sizeof(text), "%s %s", to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
		pos.x = viewport.width - line_width_px(atlas, text, text_size) - padding;
		pos.y -= atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
	}

	{   // Memory stats.
		const f32 pers_percent = (f32)pers->used / pers->size * 100.0f;
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)pers->used / 1024 / 1024, (f32)pers->size / 1024 / 1024, pers_percent);
		pos.x = padding;
		pos.y = padding;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);

		const f32 frame_percent = (f32)frame->used / frame->size * 100.0f;
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame->used / 1024 / 1024, (f32)frame->size / 1024 / 1024, frame_percent);
		pos.x = padding;
		pos.y += atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);

		const f32 temp_percent = (f32)temp->used / temp->size * 100.0f;
		text_size = (s32)stbsp_snprintf(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp->used / 1024 / 1024, (f32)temp->size / 1024 / 1024, temp_percent);
		pos.x = padding;
		pos.y += atlas->line_height;
		draw_text_with_shadow(text, text_size, pos, vec3_white, shadow_offset, vec3_black);
	}
}
