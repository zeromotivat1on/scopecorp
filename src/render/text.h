#pragma once

#include "math/matrix.h"
#include "render/render_command.h"

// Size of batch for text glyphs to draw, must be the same as in shaders (text.glsl)
inline constexpr s32 TEXT_DRAW_BATCH_SIZE = 128;

struct vec2;
struct vec3;
struct Viewport;
struct Font_Atlas;

struct Text_Render_Command : Render_Command {
	mat4 projection;
	mat4 transforms[TEXT_DRAW_BATCH_SIZE];
	u32  charmap[TEXT_DRAW_BATCH_SIZE];

	Font_Atlas *atlas = null;
};

struct Text_Draw_Buffer {
    
};

inline Text_Draw_Buffer text_draw_buffer;

void init_text_draw();
void flush_text_draw();

inline Text_Render_Command *text_draw_command = null; // default text draw command

Text_Render_Command *create_default_text_draw_command(Font_Atlas *atlas);

void draw_text(Text_Render_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color);
void draw_text_with_shadow(Text_Render_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color);

void on_viewport_resize(Text_Render_Command *command, Viewport *viewport);
