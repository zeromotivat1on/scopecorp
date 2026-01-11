#pragma once

struct Texture;

struct Glyph {
    u16 x0, y0, x1, y1; // position in baked texture atlas
    f32 xoff, yoff, xadvance;
};

struct Baked_Font_Meta {
    u32 atlas_path_size;
    u32 start_charcode;
    u32 charcode_count;
    s32 ascent;
    s32 descent;
    s32 line_gap;
    f32 px_h_scale;
    s32 px_height;
    s32 line_height;

    // Baked font atlas texture path of size atlas_path_size.
    // Baked font charcode_count Glyph's.
};

struct Font_Atlas {
    String   font_name;
    String   texture_name;
    Texture *texture;
    Glyph   *glyphs;
    s32      start_charcode;
    s32      end_charcode;
    s32      ascent;
    s32      descent;
    s32      line_gap;
    s32      line_height;
    s32      px_height;
    f32      px_h_scale;
    f32      space_xadvance;
};

struct {
    Font_Atlas *main_small;
    Font_Atlas *main_medium;
    Font_Atlas *main_big;
} global_font_atlases;

inline Table <String, Font_Atlas> font_atlas_table;

Font_Atlas *new_font_atlas(String name, Buffer contents);
Font_Atlas *get_font_atlas(String name);

f32 get_char_width_px(const Font_Atlas *atlas, char c);
f32 get_line_width_px(const Font_Atlas *atlas, String text);
