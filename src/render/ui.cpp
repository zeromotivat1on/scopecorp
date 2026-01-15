#include "ui.h"
#include "viewport.h"
#include "shader.h"
#include "material.h"
#include "input.h"
#include "window.h"
#include "world.h"
#include "vector.h"
#include "matrix.h"
#include "hash.h"
#include "profile.h"
#include "collision.h"
#include "reflection.h"
#include "hash_table.h"
#include "stb_sprintf.h"

constexpr u32 MAX_UI_INPUT_BUFFER_SIZE = Kilobytes(16);

static Render_Key get_ui_render_key(f32 z) {
    Assert(z <= UI_MAX_Z);
    
    Render_Key key;
    key.screen_layer = SCREEN_HUD_LAYER;

    const auto manager = get_entity_manager();
    const auto &camera = manager->camera;
    const f32 norm = z / UI_MAX_Z;

    Assert(norm >= 0.0f);
    Assert(norm <= 1.0f);
        
    const u32 bits = *(u32 *)&norm;
    key.depth = bits >> 8;

    return key;
}

static char *get_or_alloc_input_buffer(uiid id, u32 size) {
    auto v = table_find(ui.input_table, id);
    if (v == null) {        
        Assert(size + ui.input_buffer.size < MAX_UI_INPUT_BUFFER_SIZE);

        auto text = ui.input_buffer.data + ui.input_buffer.size;
        text[0] = '\0';
        text[size + 1] = '\0';

        ui.input_buffer.size += size + 1;
        return table_add(ui.input_table, id, (char *)text);
    }
    
    return *v;
}

void init_ui() {
    table_realloc (ui.input_table, 512);
    table_set_hash(ui.input_table, [] (const uiid &a) { return (u64)hash_pcg(a.owner + a.item + a.index); });

    ui.input_buffer.data = (u8 *)alloc(MAX_UI_INPUT_BUFFER_SIZE);
        
    {
        auto &render = ui.line_render;

        auto gpu_allocation = gpu_alloc(2 * render.MAX_LINES * sizeof(Vector2), &gpu_write_allocator);
        const auto pos_buffer_offset = gpu_allocation.offset;
        render.positions_offset = gpu_allocation.offset;
        render.positions = (Vector2 *)gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(2 * render.MAX_LINES * sizeof(u32), &gpu_write_allocator);
        const auto color_buffer_offset = gpu_allocation.offset;
        render.colors_offset = gpu_allocation.offset;
        render.colors = (Color32 *)gpu_allocation.mapped_data;

        Gpu_Vertex_Binding bindings[2];
        bindings[0].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[0].index      = 0;
        bindings[0].stride     = 8;
        bindings[1].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[1].index      = 1;
        bindings[1].stride     = 4;

        Gpu_Vertex_Attribute attributes[2];
        attributes[0].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V2;
        attributes[0].index   = 0;
        attributes[0].binding = 0;
        attributes[0].offset  = 0;
        attributes[1].type    = GPU_VERTEX_ATTRIBUTE_TYPE_U32;
        attributes[1].index   = 1;
        attributes[1].binding = 1;
        attributes[1].offset  = 0;

        render.material = ATOM("ui_element");
        render.vertex_input = gpu_new_vertex_input(bindings, carray_count(bindings), attributes, carray_count(attributes));
    }
        
    {
        auto &render = ui.quad_render;

        auto gpu_allocation = gpu_alloc(4 * render.MAX_QUADS * sizeof(Vector2), &gpu_write_allocator);
        const auto pos_buffer_offset = gpu_allocation.offset;
        render.positions_offset = gpu_allocation.offset;
        render.positions = (Vector2 *)gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(4 * render.MAX_QUADS * sizeof(u32), &gpu_write_allocator);
        const auto color_buffer_offset = gpu_allocation.offset;
        render.colors_offset = gpu_allocation.offset;
        render.colors = (Color32 *)gpu_allocation.mapped_data;

        Gpu_Vertex_Binding bindings[2];
        bindings[0].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[0].index      = 0;
        bindings[0].stride     = 8;
        bindings[1].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[1].index      = 1;
        bindings[1].stride     = 4;

        Gpu_Vertex_Attribute attributes[2];
        attributes[0].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V2;
        attributes[0].index   = 0;
        attributes[0].binding = 0;
        attributes[0].offset  = 0;
        attributes[1].type    = GPU_VERTEX_ATTRIBUTE_TYPE_U32;
        attributes[1].index   = 1;
        attributes[1].binding = 1;
        attributes[1].offset  = 0;
        
        render.material = ATOM("ui_element");
        render.vertex_input = gpu_new_vertex_input(bindings, carray_count(bindings), attributes, carray_count(attributes));
    }

    {
        auto &render = ui.text_render;
        
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        auto gpu_allocation = gpu_alloc(carray_count(vertices) * sizeof(f32), &gpu_write_allocator);
        const auto position_buffer_offset = gpu_allocation.offset;
        render.positions_offset = gpu_allocation.offset;
        render.positions = (f32 *)gpu_allocation.mapped_data;
        copy(render.positions, vertices, sizeof(vertices));

        gpu_allocation = gpu_alloc(render.MAX_CHARS * sizeof(Vector4), &gpu_write_allocator);
        const auto uv_rect_buffer_offset = gpu_allocation.offset;
        render.uv_rects_offset = gpu_allocation.offset;
        render.uv_rects = (Vector4 *)gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(render.MAX_CHARS * sizeof(u32), &gpu_write_allocator);
        const auto color_buffer_offset = gpu_allocation.offset;
        render.colors_offset = gpu_allocation.offset;
        render.colors = (Color32 *)gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(render.MAX_CHARS * sizeof(u32), &gpu_write_allocator);
        const auto charmap_buffer_offset = gpu_allocation.offset;
        render.charmap_offset = gpu_allocation.offset;
        render.charmap = (u32 *)gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(render.MAX_CHARS * sizeof(Matrix4), &gpu_write_allocator);
        const auto transform_buffer_offset = gpu_allocation.offset;
        render.transforms_offset = gpu_allocation.offset;
        render.transforms = (Matrix4 *)gpu_allocation.mapped_data;

        Gpu_Vertex_Binding bindings[5];
        bindings[0].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[0].index      = 0;
        bindings[0].stride     = 8;
        bindings[1].input_rate = GPU_VERTEX_INPUT_RATE_INSTANCE;
        bindings[1].index      = 1;
        bindings[1].stride     = 16;
        bindings[2].input_rate = GPU_VERTEX_INPUT_RATE_INSTANCE;
        bindings[2].index      = 2;
        bindings[2].stride     = 4;
        bindings[3].input_rate = GPU_VERTEX_INPUT_RATE_INSTANCE;
        bindings[3].index      = 3;
        bindings[3].stride     = 4;
        bindings[4].input_rate = GPU_VERTEX_INPUT_RATE_INSTANCE;
        bindings[4].index      = 4;
        bindings[4].stride     = 64;

        Gpu_Vertex_Attribute attributes[8];
        attributes[0].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V2;
        attributes[0].index   = 0;
        attributes[0].binding = 0;
        attributes[0].offset  = 0;
        attributes[1].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V4;
        attributes[1].index   = 1;
        attributes[1].binding = 1;
        attributes[1].offset  = 0;
        attributes[2].type    = GPU_VERTEX_ATTRIBUTE_TYPE_U32;
        attributes[2].index   = 2;
        attributes[2].binding = 2;
        attributes[2].offset  = 0;
        attributes[3].type    = GPU_VERTEX_ATTRIBUTE_TYPE_U32;
        attributes[3].index   = 3;
        attributes[3].binding = 3;
        attributes[3].offset  = 0;
        attributes[4].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V4;
        attributes[4].index   = 4;
        attributes[4].binding = 4;
        attributes[4].offset  = 0;
        attributes[5].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V4;
        attributes[5].index   = 5;
        attributes[5].binding = 4;
        attributes[5].offset  = 16;
        attributes[6].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V4;
        attributes[6].index   = 6;
        attributes[6].binding = 4;
        attributes[6].offset  = 32;
        attributes[7].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V4;
        attributes[7].index   = 7;
        attributes[7].binding = 4;
        attributes[7].offset  = 48;
        
        render.material = ATOM("ui_text");
        render.vertex_input = gpu_new_vertex_input(bindings, carray_count(bindings), attributes, carray_count(attributes));
    }
}

u16 ui_button(uiid id, String text, UI_Button_Style style) {
    const auto atlas = style.font_atlas;
    const auto width  = get_line_width_px(atlas, text);
    const auto height = atlas->line_height;
    const auto p0 = style.pos - style.padding;
    const auto p1 = Vector2(p0.x + width + 2 * style.padding.x, p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    auto text_color = style.front_color.cold;
    auto quad_color = style.back_color.cold;

    if (inside(screen_viewport.cursor_pos, p0, p1)) {
        if (id != ui.id_hot) {
            bits |= UI_HOT_BIT;
        }
        
        ui.id_hot = id;

        text_color = style.front_color.hot;
        quad_color = style.back_color.hot;

        if (down_now(MOUSE_LEFT)) {
            ui.id_active = id;
            bits |= UI_ACTIVATED_BIT;
        }
    } else {
        if (id == ui.id_hot) {
            ui.id_hot = UIID_NONE;
            bits |= UI_UNHOT_BIT;
        }        
    }
    
    if (id == ui.id_active) {
        text_color = style.front_color.active;
        quad_color = style.back_color.active;
            
        if (up_now(MOUSE_LEFT)) {
            if (id == ui.id_hot) {
                bits |= UI_FINISHED_BIT;
            }

            ui.id_active = UIID_NONE;
            bits |= UI_LOST_BIT;
        }
    }
    
    ui_quad(p0, p1, quad_color, style.z);
    ui_text(text, style.pos, text_color, style.z + F32_EPSILON, style.font_atlas);
    
    return bits;
}

u16 ui_input_text(uiid id, char *text, u32 size, UI_Input_Style style) {
    const auto atlas  = style.font_atlas;
    const auto ascent  = atlas->ascent  * atlas->px_h_scale;
    const auto descent = atlas->descent * atlas->px_h_scale;
    const auto width   = atlas->space_xadvance * size;
    const auto height  = atlas->line_height;

    const auto p0 = style.pos - style.padding;
    const auto p1 = Vector2(p0.x + width  + 2 * style.padding.x, p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    auto text_color = style.front_color.cold;
    auto quad_color = style.back_color.cold;
    auto cursor_color = style.cursor_color.cold;

    auto count = cstring_count(text);

    if (inside(screen_viewport.cursor_pos, p0, p1)) {
        if (id != ui.id_hot) {
            bits |= UI_HOT_BIT;
        }
        
        ui.id_hot = id;
        
        text_color = style.front_color.hot;
        quad_color = style.back_color.hot;
        cursor_color = style.cursor_color.hot;

        if (down_now(MOUSE_LEFT)) {
            ui.id_active = id;
            bits |= UI_ACTIVATED_BIT;
        }
    } else {
        if (id == ui.id_hot) {
            ui.id_hot = UIID_NONE;
            bits |= UI_UNHOT_BIT;
        }
    }

    if (id == ui.id_active) {
        text_color = style.front_color.active;
        quad_color = style.back_color.active;
        cursor_color = style.cursor_color.active;

        auto window = get_window();
        For (window->events) {
            if (it.type == WINDOW_EVENT_TEXT_INPUT) {
                if (count < size && Is_Printable(it.character)) {
                    text[count + 0] = it.character;
                    text[count + 1] = '\0';
                    count += 1;

                    bits |= UI_CHANGED_BIT;
                }
                
                if (count > 0 && it.character == C_BACKSPACE) {
                    text[count - 1] = '\0';
                    count -= 1;

                    bits |= UI_CHANGED_BIT;
                }

                if (it.character == C_NEW_LINE || it.character == C_CARRIAGE_RETURN) {
                    ui.id_active = UIID_NONE;
                    bits |= UI_FINISHED_BIT;
                }
            }
        }

        if (id != ui.id_hot && down_now(MOUSE_LEFT)) {
            ui.id_active = UIID_NONE;
            bits |= UI_LOST_BIT;
        }
    }

    ui_quad(p0, p1, quad_color, style.z);
    ui_text(make_string(text, count), style.pos, text_color, style.z + F32_EPSILON, style.font_atlas);

    if (id == ui.id_active) {
        const f32 offset = (f32)atlas->space_xadvance * count;
        const Vector2 p0 = Vector2(style.pos.x + offset, style.pos.y);
        const Vector2 p1 = Vector2(p0.x + 2.0f, p0.y + ascent - descent);
    
        ui_quad(p0, p1, cursor_color, style.z + F32_EPSILON);
    }
    
    return bits;
}

u16 ui_input_f32(uiid id, f32 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_F32;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (f32)string_to_float(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%.3f", *v);
    }
    
    return bits;
}

u16 ui_input_s8(uiid id, s8 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S8;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s8)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s16(uiid id, s16 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S16;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s16)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s32(uiid id, s32 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S32;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s32)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s64(uiid id, s64 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S64;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%lld", *v);
    }
    
    return bits;
}

u16 ui_input_u8(uiid id, u8 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U8;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u8)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u16(uiid id, u16 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U16;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u16)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u32(uiid id, u32 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U32;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u32)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }

    return bits;
}

u16 ui_input_u64(uiid id, u64 *v, UI_Input_Style style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U64;
    
    auto text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u64)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%llu", *v);
    }

    return bits;
}

u16 ui_combo(uiid id, u32 *selected_index, u32 option_count, const String *options, UI_Combo_Style style) {
    Assert(*selected_index < option_count);

    u16 bits = 0;
    
    const UI_Button_Style button_style = {
        style.font_atlas,
        style.z,
        style.pos,
        style.padding,
        style.margin,
        style.back_color,
        style.front_color,
    };

    const auto option = options[*selected_index];
    const auto button_bits = ui_button(id, option, button_style);
    
    if (button_bits & UI_FINISHED_BIT) {
        bits |= UI_CHANGED_BIT;
        bits |= UI_FINISHED_BIT;
        
        *selected_index += 1;
        if (*selected_index >= option_count) {
            *selected_index = 0;
        }
    }

    return bits;
}

void ui_text(String text, Vector2 pos, Color32 color, f32 z, const Font_Atlas *atlas) {
    if (!text) return;
    if (color.a == 0) return;
    
    auto &render = ui.text_render;
    Assert(render.char_count + text.size <= render.MAX_CHARS);

    const auto image_view = gpu_get_image_view(atlas->texture->image_view);
    const auto image      = gpu_get_image(image_view->image);
    
    const auto material = get_material(render.material);
    
    Render_Primitive prim;
    prim.topology = GPU_TOPOLOGY_TRIANGLE_STRIP;
    prim.shader = get_shader(material->shader);
    prim.texture = atlas->texture;
    prim.vertex_input = render.vertex_input;
    prim.vertex_offsets = New(u64, 5, __temporary_allocator);
    prim.vertex_offsets[0] = render.positions_offset;
    prim.vertex_offsets[1] = render.uv_rects_offset;
    prim.vertex_offsets[2] = render.colors_offset;
    prim.vertex_offsets[3] = render.charmap_offset;
    prim.vertex_offsets[4] = render.transforms_offset;
    prim.first_element = 0;
    prim.element_count = 4;
    prim.first_instance = render.char_count;
    prim.instance_count = 0;

    // array_reserve(prim.cb_instances, material->cbi_table.count);
    // For (material->cbi_table) {
    //     array_add_no_expand(prim.cb_instances) = it.value;
    // }

    // // @Cleanup: correct resource binding, material should have their own copies?
    // array_reserve(prim.resources, material->shader->resources.count);
    // For (material->shader->resources) {
    //     array_add_no_expand(prim.resources) = &it;
    // }
    
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < text.size; ++i) {
        if (render.char_count >= render.MAX_CHARS) break;
        
		char c = text.data[i];

		if (c == C_SPACE) {
			x += atlas->space_xadvance;
			continue;
		}

		if (c == C_TAB) {
			x += 4 * atlas->space_xadvance;
			continue;
		}

		if (c == C_NEW_LINE) {
			x = pos.x;
			y -= atlas->line_height;
			continue;
		}

        if (c < atlas->start_charcode || c > atlas->end_charcode) {
            constexpr char fallback = '?';
            //log(LOG_VERBOSE, "Got unsupported character %d for text render, using '%c' as fallback", c, fallback);
            c = fallback;
        }
        
		const auto ci = c - atlas->start_charcode;
		const auto &glyph = atlas->glyphs[ci];

		const auto cw = (f32)glyph.x1 - glyph.x0;
		const auto ch = (f32)glyph.y1 - glyph.y0;
		const auto cx = x + glyph.xoff;
		const auto cy = y - (ch + glyph.yoff);

        const auto cp = Vector3(cx, cy, 0.0f);
        const auto cs = Vector3(cw, ch, 0.0f);
        const auto transform = translate(scale(Matrix4_identity(), cs), cp);

        const auto u0 = (f32)glyph.x0 / image->width;
        const auto v0 = (f32)glyph.y0 / image->height;
        const auto u1 = (f32)glyph.x1 / image->width;
        const auto v1 = (f32)glyph.y1 / image->height;

        render.uv_rects[render.char_count] = Vector4(u0, v0, u1, v1);
        render.colors[render.char_count] = color;
        render.charmap[render.char_count] = ci;
		render.transforms[render.char_count] = transform;
        render.char_count += 1;

        prim.instance_count += 1;
        
		x += glyph.xadvance;
	}
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));
}

void ui_text_with_shadow(String text, Vector2 pos, Color32 color, Vector2 shadow_offset, Color32 shadow_color, f32 z, const Font_Atlas *atlas) {
	ui_text(text, pos + shadow_offset, shadow_color, z,               atlas);
	ui_text(text, pos,                 color,        z + F32_EPSILON, atlas);
}

void ui_quad(Vector2 p0, Vector2 p1, Color32 color, f32 z) {
    if (color.a == 0) return;

    auto &render = ui.quad_render;
    Assert(render.quad_count < render.MAX_QUADS);

    const auto material = get_material(render.material);
        
    Render_Primitive prim;
    prim.topology = GPU_TOPOLOGY_TRIANGLE_STRIP;
    prim.shader = get_shader(material->shader);
    prim.vertex_input = render.vertex_input;
    prim.vertex_offsets = New(u64, 2, __temporary_allocator);
    prim.vertex_offsets[0] = render.positions_offset;
    prim.vertex_offsets[1] = render.colors_offset;
    prim.first_element = 4 * render.quad_count;
    prim.element_count = 4;
    prim.first_instance = render.quad_count;
    prim.instance_count = 1;
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));

    const auto x0 = Min(p0.x, p1.x);
    const auto y0 = Min(p0.y, p1.y);

    const auto x1 = Max(p0.x, p1.x);
    const auto y1 = Max(p0.y, p1.y);
    
    auto vp = render.positions + 4 * render.quad_count;
    auto vc = render.colors    + 4 * render.quad_count;

    vp[0] = Vector2(x0, y0);
    vp[1] = Vector2(x1, y0);
    vp[2] = Vector2(x0, y1);    
    vp[3] = Vector2(x1, y1);

    // @Cleanup: thats a bit unfortunate that we can't use instanced vertex advance rate
    // for color, maybe there is a clever solution somewhere.
    vc[0] = color;
    vc[1] = color;
    vc[2] = color;
    vc[3] = color;
    
    render.quad_count += 1;
}

void ui_line(Vector2 start, Vector2 end, Color32 color, f32 z) {
    auto &render = ui.line_render;
    Assert(render.line_count < render.MAX_LINES);

    const auto material = get_material(render.material);
    
    Render_Primitive prim;
    prim.topology = GPU_TOPOLOGY_LINES;
    prim.shader = get_shader(material->shader);
    prim.vertex_input = render.vertex_input;
    prim.vertex_offsets = New(u64, 2, __temporary_allocator);
    prim.vertex_offsets[0] = render.positions_offset;
    prim.vertex_offsets[1] = render.colors_offset;
    prim.first_element = 2 * render.line_count;
    prim.element_count = 2;
    prim.first_instance = render.line_count;
    prim.instance_count = 1;
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));
    
    auto vp = render.positions + 2 * render.line_count;
    auto vc = render.colors    + 2 * render.line_count;
        
    vp[0] = start;
    vp[1] = end;
    
    vc[0] = color;
    vc[1] = color;

    render.line_count += 1;
}

void ui_world_line(Vector3 start, Vector3 end, Color32 color, f32 z) {
    const auto manager = get_entity_manager();
    const auto &camera = manager->camera;
    const auto rect    = make_rect(screen_viewport);
    
    const Vector2 s = world_to_screen(start, camera, rect);
    const Vector2 e = world_to_screen(end,   camera, rect);
    
    ui_line(s, e, color, z);
}
