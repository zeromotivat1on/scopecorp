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
    u32 texture_array; // @Todo: this should be an index to texture array in registry
    u32 start_charcode;
    u32 end_charcode;
    f32 px_h_scale;
    s32 line_height;
    s16 font_size; // size of glyph square bitmap
};

inline struct Draw_Command* default_text_draw_cmd = null;
void init_default_text_draw_command(Draw_Command*& cmd); // @Cleanup: get rid of *&

Font* create_font(const char* path);
Font_Atlas* bake_font_atlas(const Font* font, u32 start_charcode, u32 end_charcode, s16 font_size);
void rescale_font_atlas(Font_Atlas* atlas, s16 font_size);

void draw_text_immediate(Draw_Command* cmd, const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color);

void on_framebuffer_resize(s32 w, s32 h);

s32 line_width_px(const Font_Atlas* atlas, const char* text, s32 text_size);


