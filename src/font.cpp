#include "pch.h"
#include "font.h"
#include "log.h"
#include "memory_storage.h"
#include "os/file.h"
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

Font *create_font(const char *path) {
	Font *font = alloc_struct_persistent(Font);
	font->info = alloc_struct_persistent(stbtt_fontinfo);
	strcpy_s(font->path, sizeof(font->path), path);

	u64 buffer_size = MB(1);
	u8 *buffer = alloc_buffer_persistent(buffer_size);

	u64 data_size = 0;
	if (!read_file(path, buffer, buffer_size, &data_size)) {
		free_buffer_persistent(buffer_size);
		return null;
	}

	// Free left memory.
	if (buffer_size > data_size) free_buffer_persistent(buffer_size - data_size);

	// @Cleanup: set stbtt allocator to allocate from persistent memory storage.
	stbtt_InitFont(font->info, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));
	stbtt_GetFontVMetrics(font->info, &font->ascent, &font->descent, &font->line_gap);

	return font;
}

s32 line_width_px(const Font_Atlas *atlas, const char *text, s32 text_size) {
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

		const u32 ci = text[i] - atlas->start_charcode;
		const Font_Glyph_Metric *metric = atlas->metrics + ci;
		width += metric->advance_width;
	}
	return width;
}
