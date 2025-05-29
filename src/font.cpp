#include "pch.h"
#include "font.h"
#include "log.h"
#include "str.h"
#include "stb_truetype.h"

#include "os/file.h"

Font *create_font(const char *data) {
	Font *font = alloclt(Font);
	font->info = alloclt(stbtt_fontinfo);

    // @Cleanup: set stbtt allocator to allocate from linear allocation.
    stbtt_InitFont(font->info, (u8 *)data, stbtt_GetFontOffsetForIndex((u8 *)data, 0));
	stbtt_GetFontVMetrics(font->info, &font->ascent, &font->descent, &font->line_gap);

	return font;
}

s32 get_char_width_px(const Font_Atlas *atlas, const char c) {
    if (c == ASCII_SPACE) {
        return atlas->space_advance_width;
    }

    if (c == ASCII_TAB) {
        return 4 * atlas->space_advance_width;
    }

    if (c == ASCII_NEW_LINE) {
        return 0;
    }

    const u32 ci = c - atlas->start_charcode;
    const Font_Glyph_Metric *metric = atlas->metrics + ci;
    return metric->advance_width;
}

s32 get_line_width_px(const Font_Atlas *atlas, const char *text, s32 count) {
	s32 width = 0;
	for (s32 i = 0; i < count; ++i) {
        width += get_char_width_px(atlas, text[i]);
	}
	return width;
}

s32 get_glyph_kern_advance(const Font *font, s32 c1, s32 c2) {
    return stbtt_GetGlyphKernAdvance(font->info, c1, c2);
}
