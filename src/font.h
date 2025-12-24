#pragma once

struct Texture;

struct Font_Info {
    static constexpr u32 MAX_FILE_SIZE = Megabytes(4);
    
	struct stbtt_fontinfo *info = null;

	// Unscaled font vertical params, scale by px_h_scale from Font_Atlas.
	s32 ascent   = 0;
	s32 descent  = 0;
	s32 line_gap = 0;
};

struct Font_Glyph_Metric {
	s32 offset_x = 0;
	s32 offset_y = 0;
	s32 advance_width = 0; // already scaled
};

struct Font_Atlas {
	const Font_Info *font; // font this atlas was baked from
	Font_Glyph_Metric *metrics;
	u16 texture = 0;
	u32 start_charcode;
	u32 end_charcode;
	f32 px_h_scale;
	s32 line_height;
	s32 space_advance_width;
	s16 font_size; // size of glyph square bitmap
};

struct Glyph {
    u16 x0, y0, x1, y1; // position in baked texture atlas
    f32 xoff, yoff, xadvance;
};

struct Baked_Font_Atlas {
    String name;

    Texture *texture;

    s32 start_charcode;
    s32 end_charcode;

    s32 ascent;
    s32 descent;
    s32 line_gap;

    s32 line_height;
    s32 px_height;
    f32 px_h_scale;

    f32 space_xadvance;
    
    Glyph *glyphs;
};

struct {
    Baked_Font_Atlas *main_small;
    Baked_Font_Atlas *main_medium;
    Baked_Font_Atlas *main_big;
} global_font_atlases;

inline Font_Info Font_infos[64];
inline u32 Font_info_count = 0;

inline Table <String, Baked_Font_Atlas> font_atlas_table;

void preload_all_font_atlases();

void init_font(void *data, Font_Info &font);
void bake_font_atlas(const Font_Info &font, u32 start_charcode, u32 end_charcode, s16 font_size, Font_Atlas &atlas);
void rescale_font_atlas(Font_Atlas &atlas, s16 font_size);
f32 get_char_width_px(const Baked_Font_Atlas &atlas, char c);
f32 get_line_width_px(const Baked_Font_Atlas &atlas, String text);
s32 get_glyph_kern_advance(const Font_Info &font, s32 c1, s32 c2);
