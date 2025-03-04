#include "pch.h"
#include "profile.h"
#include "font.h"
#include "memory_storage.h"

#include "game/game.h"
#include "game/world.h"

#include "render/text.h"
#include "render/viewport.h"

#include <stdio.h>

void draw_dev_stats(const Font_Atlas *atlas, const World *world) {
	const Player &player = world->player;
	const Camera &camera = world->camera;

	static char text[256];
	const vec3 text_color = vec3(1.0f);
	const f32 padding = atlas->font_size * 0.5f;
	const vec2 shadow_offset = vec2(atlas->font_size * 0.1f, -atlas->font_size * 0.1f);
	s32 text_size = 0;
	vec2 pos;

	{   // Entity data.
		text_size = (s32)sprintf_s(text, sizeof(text), "player\n\tlocation %s\n\tvelocity %s\n\taabb %s %s", to_string(player.location), to_string(player.velocity), to_string(player.aabb.min), to_string(player.aabb.max));
		pos.x = padding;
		pos.y = (f32)viewport.height - atlas->line_height;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);

		text_size = (s32)sprintf_s(text, sizeof(text), "camera\n\teye %s\n\tat %s", to_string(camera.eye), to_string(camera.at));
		pos.x = padding;
		pos.y -= atlas->line_height * 4;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);
	}

	{   // Runtime stats.
		const f32 dt = world->dt;
		text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d %s", dt * 1000.0f, 1 / dt, viewport.width, viewport.height, build_type_name);
		pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
		pos.y = (f32)viewport.height - atlas->line_height;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);
	}

	{   // Controls.
		text_size = (s32)sprintf_s(text, sizeof(text), "F1 %s F2 %s F3 %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
		pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
		pos.y = padding;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);

		text_size = (s32)sprintf_s(text, sizeof(text), "Shift/Control + Arrows - force move/rotate game camera");
		pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
		pos.y += atlas->line_height;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);
	}

	{   // Memory stats.
		const f32 pers_percent = (f32)pers->used / pers->size * 100.0f;
		text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)pers->used / 1024 / 1024, (f32)pers->size / 1024 / 1024, pers_percent);
		pos.x = padding;
		pos.y = padding;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);

		const f32 frame_percent = (f32)frame->used / frame->size * 100.0f;
		text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame->used / 1024 / 1024, (f32)frame->size / 1024 / 1024, frame_percent);
		pos.x = padding;
		pos.y += atlas->line_height;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);

		const f32 temp_percent = (f32)temp->used / temp->size * 100.0f;
		text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp->used / 1024 / 1024, (f32)temp->size / 1024 / 1024, temp_percent);
		pos.x = padding;
		pos.y += atlas->line_height;
		draw_text_immediate_with_shadow(text_draw_command, text, text_size, pos, text_color, shadow_offset, vec3_zero);
	}
}
