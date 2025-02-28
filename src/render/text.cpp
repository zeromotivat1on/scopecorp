#include "pch.h"
#include "text.h"
#include "font.h"
#include "os/file.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "render/draw.h"
#include "render/render_registry.h"
#include "render/viewport.h"

Text_Draw_Command* create_default_text_draw_command(Font_Atlas* atlas) {
    // @Cleanup: create separate container for draw commands.
    auto* cmd = alloc_struct_persistent(Text_Draw_Command);
    *cmd = Text_Draw_Command();

    cmd->flags |= DRAW_FLAG_IGNORE_DEPTH;
    cmd->draw_mode = DRAW_TRIANGLE_STRIP;
    cmd->material_idx = material_index_list.text;
    cmd->atlas = atlas;
    set_material_uniform_value(cmd->material_idx, "u_charmap", cmd->charmap);
    set_material_uniform_value(cmd->material_idx, "u_transforms", cmd->transforms);
    set_material_uniform_value(cmd->material_idx, "u_projection", &cmd->projection);

    f32 vertices[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
    };
    Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V2 };
    cmd->vertex_buffer_idx = create_vertex_buffer(attribs, c_array_count(attribs), vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

    return cmd;
}

void draw_text_immediate(Text_Draw_Command* cmd, const char* text, u32 text_size, vec2 pos, vec3 color) {
    const auto* atlas = cmd->atlas;

    // @Cleanup: create function for this.
    render_registry.materials[cmd->material_idx].texture_idx = atlas->texture_idx;
    set_material_uniform_value(cmd->material_idx, "u_text_color", &color);

    // As charmap and transforms array are static arrays and won't be moved,
    // we can just make uniforms dirty, so they will be synced with gpu later.
    mark_material_uniform_dirty(cmd->material_idx, "u_charmap");
    mark_material_uniform_dirty(cmd->material_idx, "u_transforms");

    s32 work_idx = 0;
    f32 x = pos.x;
    f32 y = pos.y;
    
    for (u32 i = 0; i < text_size; ++i) {
        const char c = text[i];
        
        if (c == '\n') {
            x = pos.x;
            y -= atlas->line_height;
            continue;
        }
        
        assert((u32)c >= atlas->start_charcode);
        assert((u32)c <= atlas->end_charcode);

        const u32 ci = c - atlas->start_charcode; // correctly shifted index
        const Font_Glyph_Metric* metric = atlas->metrics + ci;
        
        if (c == ' ') {
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

        if (++work_idx >= TEXT_RENDER_BATCH_SIZE) {
            cmd->instance_count = work_idx;
            draw(cmd);
            work_idx = 0;
        }

        x += metric->advance_width;
    }

    if (work_idx > 0) {
        cmd->instance_count = work_idx; 
        draw(cmd);
    }
}

void draw_text_immediate_with_shadow(Text_Draw_Command* cmd, const char* text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color)
{
    draw_text_immediate(cmd, text, text_size, pos + shadow_offset, shadow_color);
    draw_text_immediate(cmd, text, text_size, pos, color);
}

void on_viewport_resize(Text_Draw_Command* cmd, Viewport* viewport) {
    cmd->projection = mat4_orthographic(0, viewport->width, 0, viewport->height, -1, 1);
    mark_material_uniform_dirty(cmd->material_idx, "u_projection");
}
