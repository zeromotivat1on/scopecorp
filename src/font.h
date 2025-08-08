#pragma once

struct Font_Info {
    static constexpr u32 MAX_FILE_SIZE = MB(4);
    
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

inline Font_Info Font_infos[64];
inline u32 Font_info_count = 0;

void init_font(void *data, Font_Info &font);
void bake_font_atlas(const Font_Info &font, u32 start_charcode, u32 end_charcode, s16 font_size, Font_Atlas &atlas);
void rescale_font_atlas(Font_Atlas &atlas, s16 font_size);
s32 get_char_width_px(const Font_Atlas &atlas, const char c);
s32 get_line_width_px(const Font_Atlas &atlas, String text);
s32 get_glyph_kern_advance(const Font_Info &font, s32 c1, s32 c2);
