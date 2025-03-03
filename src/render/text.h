#pragma once

#include "math/matrix.h"
#include "render/draw.h"

// Size of batch for text glyphs to render.
// Must be the same as in shaders (text.glsl)
inline constexpr s32 TEXT_RENDER_BATCH_SIZE = 128;

struct vec2;
struct vec3;
struct Viewport;
struct Font_Atlas;

struct Text_Draw_Command : Draw_Command {
	mat4 projection;
	mat4 transforms[TEXT_RENDER_BATCH_SIZE];
	u32  charmap[TEXT_RENDER_BATCH_SIZE];

	Font_Atlas *atlas = null;
};

inline Text_Draw_Command *text_draw_command = null; // default text draw command

Text_Draw_Command *create_default_text_draw_command(Font_Atlas *atlas);

void draw_text_immediate(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color);
void draw_text_immediate_with_shadow(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color);

void on_viewport_resize(Text_Draw_Command *command, Viewport *viewport);
