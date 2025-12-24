#include "pch.h"
#include "font.h"
#include "stb_truetype.h"
#include "file_system.h"
#include "render.h"
#include "texture.h"

#include "better_vcr_16.h"
#include "better_vcr_24.h"

// void preload_all_font_atlases() {
//     table_realloc(Font_atlas_table, 16);
    
//     {
//         constexpr String name = S("better_vcr_16");

//         auto texture = get_texture(name);        
//         auto &atlas = Font_atlas_table[name];
//         atlas.name           = name;
//         atlas.texture        = texture;
//         atlas.start_charcode = better_vcr_16_start_charcode;
//         atlas.end_charcode   = atlas.start_charcode + carray_count(better_vcr_16_cdata) - 1;
//         atlas.ascent         = better_vcr_16_ascent;
//         atlas.descent        = better_vcr_16_descent;
//         atlas.line_gap       = better_vcr_16_line_gap;
//         atlas.line_height    = better_vcr_16_line_height;
//         atlas.px_height      = 16;
//         atlas.px_h_scale     = better_vcr_16_px_h_scale;
//         atlas.space_xadvance = better_vcr_16_cdata[0].xadvance;
//         atlas.glyphs         = New(Glyph, carray_count(better_vcr_16_cdata));

//         copy(atlas.glyphs, better_vcr_16_cdata, sizeof(better_vcr_16_cdata));

//         Global_font_atlases.main_small = &atlas;
//     }

//     {
//         constexpr String name = S("better_vcr_24");

//         auto texture = get_texture(name);        
//         auto &atlas = Font_atlas_table[name];
//         atlas.name           = name;
//         atlas.texture        = texture;
//         atlas.start_charcode = better_vcr_24_start_charcode;
//         atlas.end_charcode   = atlas.start_charcode + carray_count(better_vcr_16_cdata) - 1;
//         atlas.ascent         = better_vcr_24_ascent;
//         atlas.descent        = better_vcr_24_descent;
//         atlas.line_gap       = better_vcr_24_line_gap;
//         atlas.line_height    = better_vcr_24_line_height;
//         atlas.px_height      = 24;
//         atlas.px_h_scale     = better_vcr_24_px_h_scale;
//         atlas.space_xadvance = better_vcr_24_cdata[0].xadvance;
//         atlas.glyphs         = New(Glyph, carray_count(better_vcr_24_cdata));

//         copy(atlas.glyphs, better_vcr_24_cdata, sizeof(better_vcr_24_cdata));
        
//         Global_font_atlases.main_medium = &atlas;
//     }
// }

f32 get_char_width_px(const Baked_Font_Atlas &atlas, const char c) {
    if (c == C_SPACE) {
        return atlas.space_xadvance;
    }

    if (c == C_TAB) {
        return 4 * atlas.space_xadvance;
    }

    if (c == C_NEW_LINE) {
        return 0;
    }

    Assert(Is_Ascii_Ex(c));

    const u32 ci = c - atlas.start_charcode;
    const auto &glyph = atlas.glyphs[ci];

    return glyph.xadvance;
}

f32 get_line_width_px(const Baked_Font_Atlas &atlas, String text) {
	f32 width = 0;
	for (s32 i = 0; i < text.count; ++i) {
        width += get_char_width_px(atlas, text.data[i]);
	}
	return width;
}

s32 get_glyph_kern_advance(const Font_Info &font, s32 c1, s32 c2) {
    return stbtt_GetGlyphKernAdvance(font.info, c1, c2);
}
