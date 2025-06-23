#include "pch.h"
#include "font.h"
#include "log.h"
#include "str.h"
#include "stb_truetype.h"

#include "os/file.h"

#include "render/texture.h"

Font_Info *create_font_info(void *data) {
	Font_Info *font = alloclt(Font_Info);
	font->info = alloclt(stbtt_fontinfo);

    // @Cleanup: set stbtt allocator to allocate from linear allocation.
    stbtt_InitFont(font->info, (u8 *)data, stbtt_GetFontOffsetForIndex((u8 *)data, 0));
	stbtt_GetFontVMetrics(font->info, &font->ascent, &font->descent, &font->line_gap);

	return font;
}

Font_Atlas *bake_font_atlas(const Font_Info *font, u32 start_charcode, u32 end_charcode, s16 font_size) {
	Font_Atlas *atlas = alloclt(Font_Atlas);
	atlas->font = font;

    atlas->rid_texture = r_create_texture(TEXTURE_TYPE_2D_ARRAY, TEXTURE_FORMAT_RGBA_8, font_size, font_size);
    r_set_texture_filter(atlas->rid_texture, TEXTURE_FILTER_NEAREST, false);

	const u32 charcode_count = end_charcode - start_charcode + 1;
	atlas->metrics = allocltn(Font_Glyph_Metric, charcode_count);
	atlas->start_charcode = start_charcode;
	atlas->end_charcode = end_charcode;
    
	rescale_font_atlas(atlas, font_size);

	return atlas;
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

s32 get_line_width_px(const Font_Atlas *atlas, const char *text) {
    s32 width = 0;
    const char *c = text;
	while (*c != '\0') {
        width += get_char_width_px(atlas, *c);
        c += 1;
	}
	return width;
}

s32 get_line_width_px(const Font_Atlas *atlas, const char *text, s32 count) {
	s32 width = 0;
	for (s32 i = 0; i < count; ++i) {
        width += get_char_width_px(atlas, text[i]);
	}
	return width;
}

s32 get_glyph_kern_advance(const Font_Info *font, s32 c1, s32 c2) {
    return stbtt_GetGlyphKernAdvance(font->info, c1, c2);
}
