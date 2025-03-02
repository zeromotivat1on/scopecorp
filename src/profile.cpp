#include "pch.h"
#include "profile.h"
#include "font.h"

#include "game/game.h"
#include "game/world.h"

#include "render/text.h"
#include "render/viewport.h"

#include <stdio.h>

void draw_dev_stats(const Font_Atlas* atlas, const World* world) {
    const Player* player = &world->player;
    const Camera* camera = &world->camera;
    
    static char text[256];
    const vec3 text_color = vec3(1.0f);
    const f32 padding = atlas->font_size * 0.5f;
    const vec2 shadow_offset = vec2(atlas->font_size * 0.1f, -atlas->font_size * 0.1f);
    s32 text_size = 0;
    vec2 pos;

    {   // Entity data.
        text_size = (s32)sprintf_s(text, sizeof(text), "player location %s velocity %s", to_string(player->location), to_string(player->velocity));
        pos.x = padding;
        pos.y = (f32)viewport.height - atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
        
        text_size = (s32)sprintf_s(text, sizeof(text), "camera eye %s at %s", to_string(camera->eye), to_string(camera->at));
        pos.x = padding;
        pos.y -= atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
    }
        
    {   // Runtime stats.
        const f32 dt = world->dt;
        text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d %s", dt * 1000.0f, 1 / dt, viewport.width, viewport.height, build_type_name);
        pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
        pos.y = viewport.height - (f32)atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
    }

    {   // Controls.
        text_size = (s32)sprintf_s(text, sizeof(text), "F1 %s F2 %s F3 %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
        pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
        pos.y = padding;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);

        text_size = (s32)sprintf_s(text, sizeof(text), "Shift/Control + Arrows - force move/rotate game camera");
        pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
        pos.y += atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
    }

    {   // Memory stats.
        u64 persistent_size;
        u64 persistent_used;
        usage_persistent(&persistent_size, &persistent_used);
        f32 persistent_part = (f32)persistent_used / persistent_size * 100.0f;

        text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)persistent_used / 1024 / 1024, (f32)persistent_size / 1024 / 1024, persistent_part);
        pos.x = padding;
        pos.y = padding;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
            
        u64 frame_size;
        u64 frame_used;
        usage_frame(&frame_size, &frame_used);
        f32 frame_part = (f32)frame_used / frame_size * 100.0f;

        text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame_used / 1024 / 1024, (f32)frame_size / 1024 / 1024, frame_part);
        pos.x = padding;
        pos.y += atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
            
        u64 temp_size;
        u64 temp_used;
        usage_temp(&temp_size, &temp_used);
        f32 temp_part = (f32)temp_used / temp_size * 100.0f;
            
        text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp_used / 1024 / 1024, (f32)temp_size / 1024 / 1024, temp_part);
        pos.x = padding;
        pos.y += atlas->font_size;
        draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
    }
    
}
