#include "pch.h"
#include "font.h"
#include "log.h"
#include "str.h"
#include "stb_truetype.h"

#include "os/file.h"

Font *create_font(const char *path) {
	Font *font = alloclt(Font);
	font->info = alloclt(stbtt_fontinfo);
	str_copy(font->path, path);

	u64 buffer_size = MB(1);
	u8 *buffer = allocltn(u8, buffer_size);
    
	u64 data_size = 0;
	if (!read_file(path, buffer, buffer_size, &data_size)) {
        freel(buffer_size);
		return null;
	}

	// Free left memory.
	if (buffer_size > data_size) freel(buffer_size - data_size);

	// @Cleanup: set stbtt allocator to allocate from linear allocation.
	stbtt_InitFont(font->info, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));
	stbtt_GetFontVMetrics(font->info, &font->ascent, &font->descent, &font->line_gap);

	return font;
}

s32 get_line_width_px(const Font_Atlas *atlas, const char *text, s32 text_size) {
	s32 width = 0;
	for (s32 i = 0; i < text_size; ++i) {
		if (text[i] == ' ') {
			width += atlas->space_advance_width;
			continue;
		}

		if (text[i] == '\t') {
			width += 4 * atlas->space_advance_width;
			continue;
		}

        if (text[i] == '\n') {
            width = 0;
            continue;
        }

		const u32 ci = text[i] - atlas->start_charcode;
		const Font_Glyph_Metric *metric = atlas->metrics + ci;
		width += metric->advance_width;
	}
	return width;
}

s32 get_glyph_kern_advance(const Font *font, s32 c1, s32 c2) {
    return stbtt_GetGlyphKernAdvance(font->info, c1, c2);
}
