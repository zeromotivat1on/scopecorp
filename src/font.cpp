#include "pch.h"
#include "font.h"
#include "stb_truetype.h"
#include "file_system.h"
#include "render.h"
#include "texture.h"

Font_Atlas *new_font_atlas(String name, Buffer contents) {
    auto p = (void **)&contents.data;
    
    auto meta   = *Eat(p, Baked_Font_Meta);
    auto cpath  =  Eat(p, char, meta.atlas_path_size);
    auto glyphs =  Eat(p, Glyph, meta.charcode_count);

    auto font_name  = get_file_name_no_ext(name);
    auto atlas_name = get_file_name_no_ext(make_string(cpath));
        
    auto &atlas = font_atlas_table[font_name];
    atlas.texture_name   = copy_string(atlas_name);
    atlas.font_name      = copy_string(font_name);
    atlas.start_charcode = meta.start_charcode;
    atlas.end_charcode   = meta.start_charcode + meta.charcode_count - 1;
    atlas.ascent         = meta.ascent;
    atlas.descent        = meta.descent;
    atlas.line_gap       = meta.line_gap;
    atlas.px_h_scale     = meta.px_h_scale;
    atlas.px_height      = meta.px_height;
    atlas.line_height    = meta.line_height;
    atlas.space_xadvance = glyphs[0].xadvance;
    atlas.glyphs         = New(Glyph, meta.charcode_count);

    copy(atlas.glyphs, glyphs, meta.charcode_count * sizeof(atlas.glyphs[0]));

    return &atlas;
}

Font_Atlas *get_font_atlas(String name) { return table_find(font_atlas_table, name); }

f32 get_char_width_px(const Font_Atlas *atlas, const char c) {
    if (c == C_SPACE) {
        return atlas->space_xadvance;
    }

    if (c == C_TAB) {
        return 4 * atlas->space_xadvance;
    }

    if (c == C_NEW_LINE) {
        return 0;
    }

    Assert(Is_Ascii_Ex(c));

    const u32 ci = c - atlas->start_charcode;
    const auto &glyph = atlas->glyphs[ci];

    return glyph.xadvance;
}

f32 get_line_width_px(const Font_Atlas *atlas, String text) {
	f32 width = 0;
	for (s32 i = 0; i < text.size; ++i) {
        width += get_char_width_px(atlas, text.data[i]);
	}
	return width;
}
