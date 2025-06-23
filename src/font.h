#pragma once

#include "asset.h"

inline constexpr s32 MAX_FONT_SIZE = MB(1);

struct Font : Asset {
    Font() { asset_type = ASSET_FONT; }

    void *data = null;
};

struct Font_Info {
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
	rid rid_texture = RID_NONE;
	u32 start_charcode;
	u32 end_charcode;
	f32 px_h_scale;
	s32 line_height;
	s32 space_advance_width;
	s16 font_size; // size of glyph square bitmap
};

void init_font(void *data, Font_Info *font);
void bake_font_atlas(const Font_Info *font, u32 start_charcode, u32 end_charcode, s16 font_size, Font_Atlas *atlas);
void rescale_font_atlas(Font_Atlas *atlas, s16 font_size);
s32 get_char_width_px(const Font_Atlas *atlas, const char c);
s32 get_line_width_px(const Font_Atlas *atlas, const char *text);
s32 get_line_width_px(const Font_Atlas *atlas, const char *text, s32 count);
s32 get_glyph_kern_advance(const Font_Info *font, s32 c1, s32 c2);
