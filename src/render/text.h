#pragma once

// Size of batch for text glyphs to render.
// Must be the same as in shaders.
inline constexpr s32 TEXT_RENDER_BATCH_SIZE = 128;

void draw_text_immediate(const struct Font_Atlas* atlas, const char* text, u32 text_size, struct vec2 pos, struct vec3 color);
void on_framebuffer_resize(s32 w, s32 h);
