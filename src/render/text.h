#pragma once

#include "math/matrix.h"
#include "render/render_command.h"

inline constexpr s32 MAX_CHAR_RENDER_COUNT = 1024;

struct vec2;
struct vec3;
struct Font_Atlas;

struct Text_Draw_Buffer {
    Font_Atlas *atlas = null;

    vec3 *colors     = null;
    u32  *charmap    = null;
    mat4 *transforms = null;

    s32 char_count = 0;

    s32 vertex_array_index = INVALID_INDEX;
    s32 material_index     = INVALID_INDEX;
};

inline Text_Draw_Buffer text_draw_buffer;

void init_text_draw(Font_Atlas *atlas);
void draw_text(const char *text, u32 text_size, vec2 pos, vec3 color);
void draw_text_with_shadow(const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color);
void flush_text_draw();
