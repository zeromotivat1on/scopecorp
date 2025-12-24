#include "ui.h"
#include "viewport.h"
#include "shader.h"
#include "material.h"
#include "vertex_descriptor.h"
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
    char **v = table_find(ui.input_table, id);
    if (v == null) {        
        Assert(size + ui.input_buffer.count < MAX_UI_INPUT_BUFFER_SIZE);

        char *text = ui.input_buffer.data + ui.input_buffer.count;
        text[0] = '\0';
        text[size + 1] = '\0';

        ui.input_buffer.count += size + 1;
        return table_add(ui.input_table, id, text);
    }
    
    return *v;
}

void init_ui() {
    table_realloc (ui.input_table, 512);
    table_set_hash(ui.input_table, [] (const uiid &a) { return (u64)hash_pcg(a.owner + a.item + a.index); });

    ui.input_buffer.data = (char *)alloc(MAX_UI_INPUT_BUFFER_SIZE);
        
    {
        auto &render = ui.line_render;

        const u64 pos_buffer_offset = get_gpu_buffer_mark();
        render.positions = Gpu_New(Vector2, 2 * render.MAX_LINES);
        
        const u64 color_buffer_offset = get_gpu_buffer_mark();
        render.colors = Gpu_New(u32, 2 * render.MAX_LINES);

        const Vertex_Binding bindings[] = {
            {
                .binding_index = 0,
                .offset = pos_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_2, .advance_rate = 0, .normalize = false },
                },
            },
            {
                .binding_index = 1,
                .offset = color_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_U32, .advance_rate = 0, .normalize = false },
                },
            },
        };
        
        render.material = get_material(S("ui_element"));
        render.vertex_descriptor = make_vertex_descriptor(bindings, carray_count(bindings));
    }
        
    {
        auto &render = ui.quad_render;

        const u64 pos_buffer_offset = get_gpu_buffer_mark();
        render.positions = Gpu_New(Vector2, 4 * render.MAX_QUADS);
                
        const u64 color_buffer_offset = get_gpu_buffer_mark();
        render.colors = Gpu_New(u32, 4 * render.MAX_QUADS);

        const Vertex_Binding bindings[] = {
            {
                .binding_index = 0,
                .offset = pos_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_2, .advance_rate = 0, .normalize = false },
                },
            },
            {
                .binding_index = 1,
                .offset = color_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_U32, .advance_rate = 0, .normalize = false },
                },
            },
        };
        
        render.material = get_material(S("ui_element"));
        render.vertex_descriptor = make_vertex_descriptor(bindings, carray_count(bindings));
    }

    {
        auto &render = ui.text_render;
        
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        const u64 position_buffer_offset = get_gpu_buffer_mark();
        render.positions = Gpu_New(f32, carray_count(vertices));

        const u64 uv_rect_buffer_offset = get_gpu_buffer_mark();
        render.uv_rects = Gpu_New(Vector4, render.MAX_CHARS);

        const u64 color_buffer_offset = get_gpu_buffer_mark();
        render.colors = Gpu_New(u32, render.MAX_CHARS);
        
        const u64 charmap_buffer_offset = get_gpu_buffer_mark();
        render.charmap = Gpu_New(u32, render.MAX_CHARS);
        
        const u64 transform_buffer_offset = get_gpu_buffer_mark();
        render.transforms = Gpu_New(Matrix4, render.MAX_CHARS);

        copy(render.positions, vertices, sizeof(vertices));

        const Vertex_Binding bindings[] = {
            {
                .binding_index = 0,
                .offset = position_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_2, .advance_rate = 0, .normalize = false },
                },
            },
            {
                .binding_index = 1,
                .offset = uv_rect_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_4, .advance_rate = 1, .normalize = false },
                },
            },
            {
                .binding_index = 2,
                .offset = color_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_U32, .advance_rate = 1, .normalize = false },
                },
            },
            {
                .binding_index = 3,
                .offset = charmap_buffer_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_U32, .advance_rate = 1, .normalize = false },
                },
            },
            {
                .binding_index = 4,
                .offset = transform_buffer_offset,
                .component_count = 4,
                .components = {
                    { .type = VC_F32_4, .advance_rate = 1, .normalize = false },
                    { .type = VC_F32_4, .advance_rate = 1, .normalize = false },
                    { .type = VC_F32_4, .advance_rate = 1, .normalize = false },
                    { .type = VC_F32_4, .advance_rate = 1, .normalize = false },
                },
            },
        };
        
        render.material = get_material(S("ui_text"));
        render.vertex_descriptor = make_vertex_descriptor(bindings, carray_count(bindings));
    }
}

u16 ui_button(uiid id, String text, const UI_Button_Style &style) {
    const auto &atlas = *style.font_atlas;
    const f32 width = get_line_width_px(atlas, text);
    const s32 height = atlas.line_height;
    const Vector2 p0 = style.pos - style.padding;
    const Vector2 p1 = Vector2(p0.x + width + 2 * style.padding.x, p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    u32 text_color = style.front_color.cold;
    u32 quad_color = style.back_color.cold;

    if (inside(screen_viewport.mouse_pos, p0, p1)) {
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

u16 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style) {
    const auto &atlas = *style.font_atlas;
    const f32 ascent  = atlas.ascent  * atlas.px_h_scale;
    const f32 descent = atlas.descent * atlas.px_h_scale;
    const f32 width = atlas.space_xadvance * size;
    const s32 height = atlas.line_height;

    const Vector2 p0 = style.pos - style.padding;
    const Vector2 p1 = Vector2(p0.x + width  + 2 * style.padding.x,
                         p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    u32 text_color = style.front_color.cold;
    u32 quad_color = style.back_color.cold;
    u32 cursor_color = style.cursor_color.cold;

    u32 count = (u32)String(text).count;

    if (inside(screen_viewport.mouse_pos, p0, p1)) {
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
    ui_text(String { text, count }, style.pos, text_color, style.z + F32_EPSILON, style.font_atlas);

    if (id == ui.id_active) {
        const f32 offset = (f32)atlas.space_xadvance * count;
        const Vector2 p0 = Vector2(style.pos.x + offset, style.pos.y);
        const Vector2 p1 = Vector2(p0.x + 2.0f, p0.y + ascent - descent);
    
        ui_quad(p0, p1, cursor_color, style.z + F32_EPSILON);
    }
    
    return bits;
}

u16 ui_input_f32(uiid id, f32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_F32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (f32)string_to_float(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%.3f", *v);
    }
    
    return bits;
}

u16 ui_input_s8(uiid id, s8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s8)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s16(uiid id, s16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s16)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s32(uiid id, s32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (s32)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s64(uiid id, s64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%lld", *v);
    }
    
    return bits;
}

u16 ui_input_u8(uiid id, u8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u8)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u16(uiid id, u16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u16)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u32(uiid id, u32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u32)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }

    return bits;
}

u16 ui_input_u64(uiid id, u64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = (u64)string_to_integer(make_string(text, size));
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%llu", *v);
    }

    return bits;
}

u16 ui_combo(uiid id, u32 *selected_index, u32 option_count, const String *options, const UI_Combo_Style &style) {
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

void ui_text(String text, Vector2 pos, u32 color, f32 z, const Baked_Font_Atlas *font_atlas) {
    if (!text) return;
    if (rgba_get_a(color) == 0) return;
    
    auto &trender = ui.text_render;
    Assert(trender.char_count + text.count <= trender.MAX_CHARS);

	const auto &atlas = *font_atlas;

    auto material = trender.material;
    
    Render_Primitive prim;
    prim.topology_mode = TOPOLOGY_TRIANGLE_STRIP;
    prim.shader = material->shader;
    prim.texture = atlas.texture;
    prim.vertex_descriptor = &trender.vertex_descriptor;
    prim.first_element = 0;
    prim.element_count = 4;
    prim.first_instance = trender.char_count;
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

	for (u32 i = 0; i < text.count; ++i) {
        if (trender.char_count >= trender.MAX_CHARS) break;
        
		char c = text.data[i];

		if (c == C_SPACE) {
			x += atlas.space_xadvance;
			continue;
		}

		if (c == C_TAB) {
			x += 4 * atlas.space_xadvance;
			continue;
		}

		if (c == C_NEW_LINE) {
			x = pos.x;
			y -= atlas.line_height;
			continue;
		}

        if (c < atlas.start_charcode || c > atlas.end_charcode) {
            constexpr char fallback = '?';
            log(LOG_VERBOSE, "Got unsupported character %d for text render, using '%c' as fallback", c, fallback);
            c = fallback;
        }
        
		const auto ci = c - atlas.start_charcode;
		const auto &glyph = atlas.glyphs[ci];

		const auto cw = (f32)glyph.x1 - glyph.x0;
		const auto ch = (f32)glyph.y1 - glyph.y0;
		const auto cx = x + glyph.xoff;
		const auto cy = y - (ch + glyph.yoff);

        const auto cp = Vector3(cx, cy, 0.0f);
        const auto cs = Vector3(cw, ch, 0.0f);
        const auto transform = translate(scale(Matrix4_identity(), cs), cp);

        const auto u0 = (f32)glyph.x0 / atlas.texture->width;
        const auto v0 = (f32)glyph.y0 / atlas.texture->height;
        const auto u1 = (f32)glyph.x1 / atlas.texture->width;
        const auto v1 = (f32)glyph.y1 / atlas.texture->height;

        trender.uv_rects[trender.char_count] = Vector4(u0, v0, u1, v1);
        trender.colors[trender.char_count] = color;
        trender.charmap[trender.char_count] = ci;
		trender.transforms[trender.char_count] = transform;
        trender.char_count += 1;

        prim.instance_count += 1;
        
		x += glyph.xadvance;
	}
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));
}

void ui_text_with_shadow(String text, Vector2 pos, u32 color, Vector2 shadow_offset, u32 shadow_color, f32 z, const Baked_Font_Atlas *font_atlas) {
	ui_text(text, pos + shadow_offset, shadow_color, z,               font_atlas);
	ui_text(text, pos,                 color,        z + F32_EPSILON, font_atlas);
}

void ui_quad(Vector2 p0, Vector2 p1, u32 color, f32 z) {
    if (rgba_get_a(color) == 0) return;

    auto &qrender = ui.quad_render;
    Assert(qrender.quad_count < qrender.MAX_QUADS);

    auto material = qrender.material;
    
    Render_Primitive prim;
    prim.topology_mode = TOPOLOGY_TRIANGLE_STRIP;
    prim.shader = material->shader;
    prim.vertex_descriptor = &qrender.vertex_descriptor;
    prim.first_element = 4 * qrender.quad_count;
    prim.element_count = 4;
    prim.first_instance = qrender.quad_count;
    prim.instance_count = 1;
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));

    const auto x0 = Min(p0.x, p1.x);
    const auto y0 = Min(p0.y, p1.y);

    const auto x1 = Max(p0.x, p1.x);
    const auto y1 = Max(p0.y, p1.y);
    
    auto vp = qrender.positions + 4 * qrender.quad_count;
    auto vc = qrender.colors    + 4 * qrender.quad_count;

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
    
    qrender.quad_count += 1;
}

void ui_line(Vector2 start, Vector2 end, u32 color, f32 z) {
    auto &lrender = ui.line_render;
    Assert(lrender.line_count < lrender.MAX_LINES);

    auto material = lrender.material;
    
    Render_Primitive prim;
    prim.topology_mode = TOPOLOGY_LINES;
    prim.shader = material->shader;
    prim.vertex_descriptor = &lrender.vertex_descriptor;
    prim.first_element = 2 * lrender.line_count;
    prim.element_count = 2;
    prim.first_instance = lrender.line_count;
    prim.instance_count = 1;
    
    auto hud_batch = get_hud_batch();
    add_primitive(hud_batch, prim, get_ui_render_key(z));
    
    Vector2 *vp = lrender.positions + 2 * lrender.line_count;
    u32  *vc = lrender.colors    + 2 * lrender.line_count;
        
    vp[0] = start;
    vp[1] = end;
    
    vc[0] = color;
    vc[1] = color;

    lrender.line_count += 1;
}

void ui_world_line(Vector3 start, Vector3 end, u32 color, f32 z) {
    const auto manager = get_entity_manager();
    const auto &camera = manager->camera;
    const auto rect    = make_rect(screen_viewport);
    
    const Vector2 s = world_to_screen(start, camera, rect);
    const Vector2 e = world_to_screen(end,   camera, rect);
    
    ui_line(s, e, color, z);
}
