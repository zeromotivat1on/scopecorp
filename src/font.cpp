#include "pch.h"
#include "font.h"
#include "os/file.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "render/gl.h"
#include "render/shader.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

Font_Render_Context* create_font_render_context(s32 window_w, s32 window_h)
{
    Font_Render_Context* ctx = alloc_struct_persistent(Font_Render_Context);
    ctx->program = shaders.text.id;
    on_framebuffer_resize(ctx, window_w, window_h);
    
    ctx->u_charmap = glGetUniformLocation(ctx->program, "u_charmap");
    ctx->u_transforms = glGetUniformLocation(ctx->program, "u_transforms");
    ctx->u_text_color = glGetUniformLocation(ctx->program, "u_text_color");
    
    ctx->charmap = alloc_array_persistent(FONT_RENDER_BATCH_SIZE, u32);
    ctx->transforms = alloc_array_persistent(FONT_RENDER_BATCH_SIZE, mat4);
    
    glGenVertexArrays(1, &ctx->vao);
    glGenBuffers(1, &ctx->vbo);
    
    glBindVertexArray(ctx->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);

    f32 vertices[4 * 2] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return ctx;
}

Font* create_font(const char* path)
{
    Font* font = alloc_struct_persistent(Font);
    font->info = alloc_struct_persistent(stbtt_fontinfo);
    
    u64 buffer_size = MB(1);
    u8* buffer = alloc_buffer_persistent(buffer_size);

    u64 data_size = 0;
    // @Cleanup: make and use read file persistent overload.
    if (!read_file(path, buffer, buffer_size, &data_size))
    {
        free_buffer_persistent(buffer_size);
        return null;
    }

    // Free left memory.
    if (buffer_size > data_size)
        free_buffer_persistent(buffer_size - data_size);

    // @Cleanup: set stbtt allocator to allocate from persistent memory storage.
    stbtt_InitFont(font->info, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));
    stbtt_GetFontVMetrics(font->info, &font->ascent, &font->descent, &font->line_gap);
    
    return font;
}

Font_Atlas* bake_font_atlas(const Font* font, u32 start_charcode, u32 end_charcode, s16 font_size)
{
    Font_Atlas* atlas = alloc_struct_persistent(Font_Atlas);
    atlas->font = font;
        
    const u32 charcode_count = end_charcode - start_charcode + 1;
    atlas->metrics = alloc_array_persistent(charcode_count, Font_Glyph_Metric);
    atlas->start_charcode = start_charcode;
    atlas->end_charcode = end_charcode;
    
    glGenTextures(1, &atlas->texture_array);
    rescale_font_atlas(atlas, font_size);

    return atlas;
}

void rescale_font_atlas(Font_Atlas* atlas, s16 font_size)
{
    const Font* font = atlas->font;
    
    atlas->font_size = font_size;

    const u32 charcode_count = atlas->end_charcode - atlas->start_charcode + 1;
    const f32 scale = stbtt_ScaleForPixelHeight(font->info, (f32)font_size);
    atlas->px_h_scale = scale;
    atlas->line_height = (s32)((font->ascent - font->descent + font->line_gap) * scale);
    
    s32 max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
    assert(charcode_count <= (u32)max_layers);

    // stbtt rasterizes glyphs as 8bpp, so tell open gl to use 1 byte per color channel.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->texture_array);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, atlas->font_size, atlas->font_size, charcode_count, 0, GL_RED, GL_UNSIGNED_BYTE, null);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    u8* bitmap = alloc_buffer_temp(font_size * font_size);    
    for (u32 i = 0; i < charcode_count; ++i)
    {
        const u32 c = i + atlas->start_charcode;
        Font_Glyph_Metric* metric = atlas->metrics + i;
        
        s32 w, h, offx, offy;
        const s32 glyph_index = stbtt_FindGlyphIndex(font->info, c);
        u8* stb_bitmap = stbtt_GetGlyphBitmap(font->info, scale, scale, glyph_index, &w, &h, &offx, &offy);

        // Offset original bitmap to be at center of new one.
        const s32 x_offset = (font_size - w) / 2;
        const s32 y_offset = (font_size - h) / 2;

        metric->offset_x = offx - x_offset;
        metric->offset_y = offy - y_offset;
        
        memset(bitmap, 0, font_size * font_size);

        // @Cleanup: looks nasty, should come up with better solution.
        // Now we bake bitmap using stbtt and 'rescale' it to font size square one.
        // Maybe use stbtt_MakeGlyphBitmap at least?
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                int src_index = y * w + x;
                int dest_index = (y + y_offset) * font_size + (x + x_offset);
                if (dest_index >= 0 && dest_index < font_size * font_size)
                    bitmap[dest_index] = stb_bitmap[src_index];
            }
        }

        stbtt_FreeBitmap(stb_bitmap, null);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, font_size, font_size, 1, GL_RED, GL_UNSIGNED_BYTE, bitmap);

        s32 advance_width = 0;
        stbtt_GetGlyphHMetrics(font->info, glyph_index, &advance_width, 0);
        metric->advance_width = (s32)(advance_width * scale);
    }

    free_buffer_temp(font_size * font_size);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default color channel size
}

static void render_glyphs(const Font_Render_Context* render_ctx, s32 count)
{
    glUniformMatrix4fv(render_ctx->u_transforms, count, GL_FALSE, (f32*)render_ctx->transforms);
    glUniform1uiv(render_ctx->u_charmap, count, render_ctx->charmap);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
}

void render_text(const Font_Render_Context* ctx, const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color)
{
    glUseProgram(ctx->program);
    glBindVertexArray(ctx->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->texture_array);
    
    glActiveTexture(GL_TEXTURE0);
    glUniform3f(glGetUniformLocation(ctx->program, "u_text_color"), color.x, color.y, color.z);

    s32 work_idx = 0;
    f32 x = pos.x;
    f32 y = pos.y;
    
    for (u32 i = 0; i < text_size; ++i)
    {
        const char c = text[i];
        
        if (c == '\n')
        {
            x = pos.x;
            y -= atlas->line_height;
            continue;
        }
        
        assert((u32)c >= atlas->start_charcode);
        assert((u32)c <= atlas->end_charcode);

        const u32 ci = c - atlas->start_charcode; // correctly shifted index
        const Font_Glyph_Metric* metric = atlas->metrics + ci;
        
        if (c == ' ')
        {
            x += metric->advance_width;
            continue;
        }
        
        const f32 gw = (f32)atlas->font_size;
        const f32 gh = (f32)atlas->font_size;
        const f32 gx = x + metric->offset_x;
        const f32 gy = y - (gh + metric->offset_y);
        
        mat4* transform = ctx->transforms + work_idx;
        transform->identity().translate(vec3(gx, gy, 0.0f)).scale(vec3(gw, gh, 0.0f));

        ctx->charmap[work_idx] = ci;

        if (++work_idx >= FONT_RENDER_BATCH_SIZE)
        {
            render_glyphs(ctx, work_idx);
            work_idx = 0;
        }

        x += metric->advance_width;
    }

    if (work_idx > 0) render_glyphs(ctx, work_idx);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}


void on_framebuffer_resize(const Font_Render_Context* ctx, s32 w, s32 h)
{
    glUseProgram(ctx->program);
    const mat4 projection = mat4_orthographic(0.0f, (f32)w, 0.0f, (f32)h, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(ctx->program, "u_projection"), 1, GL_FALSE, (f32*)&projection);
    glUseProgram(0);
}

s32 line_width_px(const Font_Atlas* atlas, const char* text, s32 text_size)
{
    s32 width = 0;
    for (s32 i = 0; i < text_size; ++i)
    {
        const u32 ci = text[i] - atlas->start_charcode;
        const Font_Glyph_Metric* metric = atlas->metrics + ci;
        width += metric->advance_width;
    }
    return width;
}
