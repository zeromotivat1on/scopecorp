#pragma once

// Size of batch for text glyphs to render.
// Must be the same as in shaders.
inline constexpr s16 FONT_RENDER_BATCH_SIZE = 128;

struct vec2;
struct vec3;
struct mat4;
struct Gap_Buffer;

struct Font
{
    struct stbtt_fontinfo* info;
    // Unscaled font vertical params, scale by px_h_scale from Font_Atlas.
    s32 ascent;
    s32 descent;
    s32 line_gap;
};

struct Font_Glyph_Metric
{
    s32 offset_x;
    s32 offset_y;
    s32 advance_width; // already scaled
};

struct Font_Atlas
{
    const Font* font; // font this atlas was baked from
    Font_Glyph_Metric* metrics;
    u32 texture_array;
    u32 start_charcode;
    u32 end_charcode;
    f32 px_h_scale;
    s32 line_height;
    s16 font_size; // size of glyph square bitmap
};

// @Cleanup: use indices to render resources.
struct Font_Render_Context
{
    u32 program;
    u32 vao;
    u32 vbo;
    u32 u_charmap;
    u32 u_transforms;
    u32 u_text_color;
    u32* charmap;
    mat4* transforms;
};

Font_Render_Context* create_font_render_context(s32 window_w, s32 window_h);
Font* create_font(const char* path);
Font_Atlas* bake_font_atlas(const Font* font, u32 start_charcode, u32 end_charcode, s16 font_size);
void rescale_font_atlas(Font_Atlas* atlas, s16 font_size);
void render_text(const Font_Render_Context* ctx, const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color);
void on_framebuffer_resize(const Font_Render_Context* ctx, s32 w, s32 h);
s32 line_width_px(const Font_Atlas* atlas, const char* text, s32 text_size);
