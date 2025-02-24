#include "pch.h"
#include "text.h"
#include "font.h"
#include "os/file.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "render/gl.h"
#include "render/draw.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/vertex_buffer.h"
#include "render/render_registry.h"

// @Cleanup: expose to header?
struct Text_Draw_Command : Draw_Command
{
    mat4 projection;
    mat4 transforms[TEXT_RENDER_BATCH_SIZE];
    u32  charmap[TEXT_RENDER_BATCH_SIZE];
};

static Text_Draw_Command* text_draw_cmd_immediate = null;

Text_Draw_Command* create_text_draw_command()
{
    Text_Draw_Command* cmd = alloc_struct_persistent(Text_Draw_Command);
    *cmd = Text_Draw_Command();

    cmd->flags |= DRAW_FLAG_IGNORE_DEPTH;
    cmd->draw_mode = DRAW_TRIANGLE_STRIP;
    cmd->shader_idx = shader_index_list.text;
    set_shader_uniform_value(cmd->shader_idx, "u_charmap", cmd->charmap);
    set_shader_uniform_value(cmd->shader_idx, "u_transforms", cmd->transforms);
    set_shader_uniform_value(cmd->shader_idx, "u_projection", &cmd->projection);

    f32 vertices[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };
    Vertex_Attrib_Type attrib_types[] = { VERTEX_ATTRIB_F32_V2 };
    cmd->vertex_buffer_idx = create_vertex_buffer(attrib_types, c_array_count(attrib_types), vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

    return cmd;
}

void draw_text_immediate(const Font_Atlas* atlas, const char* text, u32 text_size, vec2 pos, vec3 color)
{
    if (!text_draw_cmd_immediate) text_draw_cmd_immediate = create_text_draw_command();

    auto* cmd = text_draw_cmd_immediate;
    cmd->texture_idx = atlas->texture_idx;

    set_shader_uniform_value(cmd->shader_idx, "u_text_color", &color);

    // As charmap and transforms array are static arrays and won't be moved,
    // we can just make uniforms dirty, so they will be synced with gpu later.
    mark_shader_uniform_dirty(cmd->shader_idx, "u_charmap");
    mark_shader_uniform_dirty(cmd->shader_idx, "u_transforms");

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
        
        mat4* transform = cmd->transforms + work_idx;
        transform->identity().translate(vec3(gx, gy, 0.0f)).scale(vec3(gw, gh, 0.0f));

        cmd->charmap[work_idx] = ci;

        if (++work_idx >= TEXT_RENDER_BATCH_SIZE)
        {
            cmd->instance_count = work_idx;
            draw(cmd);
            work_idx = 0;
        }

        x += metric->advance_width;
    }

    if (work_idx > 0)
    {
        cmd->instance_count = work_idx; 
        draw(cmd);
    }
}

void on_framebuffer_resize(s32 w, s32 h)
{
    if (!text_draw_cmd_immediate) text_draw_cmd_immediate = create_text_draw_command();

    text_draw_cmd_immediate->projection = mat4_orthographic(0.0f, (f32)w, 0.0f, (f32)h, -1.0f, 1.0f);
    mark_shader_uniform_dirty(text_draw_cmd_immediate->shader_idx, "u_projection");
}
