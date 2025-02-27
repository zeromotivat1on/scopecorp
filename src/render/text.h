#pragma once

// Size of batch for text glyphs to render.
// Must be the same as in shaders.
inline constexpr s32 TEXT_RENDER_BATCH_SIZE = 128;

struct vec2;
struct vec3;
struct Font_Atlas;

void draw_text_immediate(const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color);
void draw_text_immediate_with_shadow(const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color);
void on_framebuffer_resize(s32 w, s32 h);
