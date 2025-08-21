#include "pch.h"
#include "font.h"
#include "log.h"
#include "stb_truetype.h"

#include "os/file.h"

#include "render/render.h"
#include "render/r_texture.h"

void init_font(void *data, Font_Info &font) {
    Assert(font.info == null);
    
    // @Cleanup: use own arena?
	font.info = arena_push_type(M_global, stbtt_fontinfo);

    // @Cleanup: set stbtt allocator to allocate from linear allocation.
    stbtt_InitFont(font.info, (u8 *)data, stbtt_GetFontOffsetForIndex((u8 *)data, 0));
	stbtt_GetFontVMetrics(font.info, &font.ascent, &font.descent, &font.line_gap);
}

void bake_font_atlas(const Font_Info &font, u32 start_charcode, u32 end_charcode, s16 font_size, Font_Atlas &atlas) {
	atlas.font = &font;

    atlas.texture = r_create_texture(R_TEXTURE_2D_ARRAY, R_RGBA_8, font_size, font_size,
                                      R_CLAMP, R_NEAREST, R_NEAREST);

	const u32 charcode_count = end_charcode - start_charcode + 1;
    // @Cleanup: use own arena?
	atlas.metrics = arena_push_array(M_global, charcode_count, Font_Glyph_Metric);
	atlas.start_charcode = start_charcode;
	atlas.end_charcode = end_charcode;
    
	rescale_font_atlas(atlas, font_size);
}

s32 get_char_width_px(const Font_Atlas &atlas, const char c) {
    if (c == ASCII_SPACE) {
        return atlas.space_advance_width;
    }

    if (c == ASCII_TAB) {
        return 4 * atlas.space_advance_width;
    }

    if (c == ASCII_NEW_LINE) {
        return 0;
    }

    const u32 ci = c - atlas.start_charcode;
    const Font_Glyph_Metric *metric = atlas.metrics + ci;
    return metric->advance_width;
}

s32 get_line_width_px(const Font_Atlas &atlas, String text) {
	s32 width = 0;
	for (s32 i = 0; i < text.length; ++i) {
        width += get_char_width_px(atlas, text.value[i]);
	}
	return width;
}

s32 get_glyph_kern_advance(const Font_Info &font, s32 c1, s32 c2) {
    return stbtt_GetGlyphKernAdvance(font.info, c1, c2);
}
