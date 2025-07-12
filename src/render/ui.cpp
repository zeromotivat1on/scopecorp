#include "render/ui.h"
#include "render/viewport.h"
#include "render/buffer_storage.h"
#include "render/render_command.h"

#include "os/input.h"
#include "os/window.h"

#include "math/vector.h"
#include "math/matrix.h"

#include "str.h"
#include "hash.h"
#include "profile.h"
#include "collision.h"
#include "reflection.h"

#include "stb_sprintf.h"

constexpr u32 MAX_UI_INPUT_TABLE_SIZE  = 512;
constexpr u32 MAX_UI_INPUT_BUFFER_SIZE = KB(16);

typedef Hash_Table<uiid, char *> UI_Input_Table;

static UI_Input_Table ui_input_table = {};
static char *ui_input_buffer = null; // storage for input buffers
static u32 ui_input_buffer_size = 0;

static u64 ui_input_table_hash(const uiid &a) {
    return hash_pcg64(a.owner + a.item + a.index);
}

static char *get_or_alloc_input_buffer(uiid id, u32 size) {
    char **v = find(ui_input_table, id);
    if (v == null) {        
        Assert(size + ui_input_buffer_size < MAX_UI_INPUT_BUFFER_SIZE);

        char *text = ui_input_buffer + ui_input_buffer_size;
        text[0] = '\0';
        text[size + 1] = '\0';
        
        v = add(ui_input_table, id, text);
        ui_input_buffer_size += size + 1;
    }
    
    return *v;
}

void ui_init() {
    ui_input_table = UI_Input_Table(MAX_UI_INPUT_TABLE_SIZE);
    ui_input_table.hash_function = &ui_input_table_hash;

    ui_input_buffer = (char *)allocl(MAX_UI_INPUT_BUFFER_SIZE);
    
    {
        init_font(asset_table.fonts[SID_FONT_BETTER_VCR].data, &ui_default_font);

        Font_Atlas atlas_16, atlas_24;
        bake_font_atlas(&ui_default_font, 33, 126, 16, &atlas_16);
        bake_font_atlas(&ui_default_font, 33, 126, 24, &atlas_24);
        
        ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX]       = atlas_16;
        ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX] = atlas_16;
        ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX]      = atlas_16;
        ui.font_atlases[UI_SCREEN_REPORT_FONT_ATLAS_INDEX] = atlas_24;
    }
    
    {   // Text draw buffer.
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        constexpr u32 position_buffer_size  = sizeof(vertices);
        constexpr u32 color_buffer_size     = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(u32);
        constexpr u32 charmap_buffer_size   = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(u32);
        constexpr u32 transform_buffer_size = MAX_UI_TEXT_DRAW_BUFFER_CHARS * sizeof(mat4);

        const u32 position_buffer_offset  = vertex_buffer_storage.size;
        const u32 color_buffer_offset     = position_buffer_offset + position_buffer_size;
        const u32 charmap_buffer_offset   = color_buffer_offset + color_buffer_size;
        const u32 transform_buffer_offset = charmap_buffer_offset + charmap_buffer_size;

        auto &tdb = ui.text_draw_buffer;
        tdb.positions  = (f32  *)r_allocv(position_buffer_size); 
        tdb.colors     = (u32  *)r_allocv(color_buffer_size);
        tdb.charmap    = (u32  *)r_allocv(charmap_buffer_size);
        tdb.transforms = (mat4 *)r_allocv(transform_buffer_size);

        copy_bytes(tdb.positions, vertices, position_buffer_size);
        
        Vertex_Array_Binding bindings[4] = {};
        bindings[0].binding_index = 0;
        bindings[0].data_offset = position_buffer_offset;
        bindings[0].layout_size = 1;
        bindings[0].layout[0] = { VERTEX_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].data_offset = color_buffer_offset;
        bindings[1].layout_size = 1;
        bindings[1].layout[0] = { VERTEX_U32, 1 };

        bindings[2].binding_index = 2;
        bindings[2].data_offset = charmap_buffer_offset;
        bindings[2].layout_size = 1;
        bindings[2].layout[0] = { VERTEX_U32, 1 };

        bindings[3].binding_index = 3;
        bindings[3].data_offset = transform_buffer_offset;
        bindings[3].layout_size = 4;
        bindings[3].layout[0] = { VERTEX_F32_4, 1 };
        bindings[3].layout[1] = { VERTEX_F32_4, 1 };
        bindings[3].layout[2] = { VERTEX_F32_4, 1 };
        bindings[3].layout[3] = { VERTEX_F32_4, 1 };

        tdb.rid_vertex_array = r_create_vertex_array(bindings, COUNT(bindings));
        tdb.sid_material = SID_MATERIAL_UI_TEXT;
    }

    {   // Quad draw buffer.
        constexpr u32 pos_buffer_size   = 4 * MAX_UI_QUAD_DRAW_BUFFER_QUADS * sizeof(vec2);
        constexpr u32 color_buffer_size = 1 * MAX_UI_QUAD_DRAW_BUFFER_QUADS * sizeof(u32);

        const u32 pos_buffer_offset   = vertex_buffer_storage.size;
        const u32 color_buffer_offset = pos_buffer_offset + pos_buffer_size;
        
        auto &qdb = ui.quad_draw_buffer;
        qdb.positions = (vec2 *)r_allocv(pos_buffer_size);
        qdb.colors    = (u32  *)r_allocv(color_buffer_size);
        
        Vertex_Array_Binding bindings[2] = {};
        bindings[0].binding_index = 0;
        bindings[0].data_offset = pos_buffer_offset;
        bindings[0].layout_size = 1;
        bindings[0].layout[0] = { VERTEX_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].data_offset = color_buffer_offset;
        bindings[1].layout_size = 1;
        bindings[1].layout[0] = { VERTEX_U32, 0 };
        
        qdb.rid_vertex_array = r_create_vertex_array(bindings, COUNT(bindings));
        qdb.sid_material = SID_MATERIAL_UI_ELEMENT;
    }
}

static s32 cb_compare_ui_draw_cmd(const void *a, const void *b) {
    const auto &ax = *(const UI_Draw_Command *)a;
    const auto &bx = *(const UI_Draw_Command *)b;

    if (ax.z < bx.z) return -1;
    if (ax.z > bx.z) return  1;

    return 0;
}

void ui_flush() {
    PROFILE_SCOPE(__FUNCTION__);

    //sort(ui.draw_queue, ui.draw_queue_size, sizeof(ui.draw_queue[0]), cb_compare_ui_draw_cmd);
    
    auto &tdb = ui.text_draw_buffer;
    auto &qdb = ui.quad_draw_buffer;

    s32 total_char_instance_offset = 0;
    s32 total_quad_instance_offset = 0;
    
    for (s32 i = 0; i < ui.draw_queue_size; ++i) {
        Render_Command r_cmd = {};
        r_cmd.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_BLEND | RENDER_FLAG_RESET;
        r_cmd.render_mode  = RENDER_TRIANGLE_STRIP;
        r_cmd.polygon_mode = POLYGON_FILL;
        r_cmd.viewport.x      = viewport.x;
        r_cmd.viewport.y      = viewport.y;
        r_cmd.viewport.width  = viewport.width;
        r_cmd.viewport.height = viewport.height;
        r_cmd.scissor.x      = viewport.x;
        r_cmd.scissor.y      = viewport.y;
        r_cmd.scissor.width  = viewport.width;
        r_cmd.scissor.height = viewport.height;
        r_cmd.cull_face.type    = CULL_FACE_BACK;
        r_cmd.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
        r_cmd.blend.source      = BLEND_SOURCE_ALPHA;
        r_cmd.blend.destination = BLEND_ONE_MINUS_SOURCE_ALPHA;
        r_cmd.instance_count = 0;
        r_cmd.buffer_element_count = 0;
    
        const s32 char_instance_offset = total_char_instance_offset;
        const s32 quad_instance_offset = total_quad_instance_offset;
        
        const auto &ui_draw_type = ui.draw_queue[i].type;
        const auto &atlas_index  = ui.draw_queue[i].atlas_index;
        
        // Detect similar adjacent ui commands and stack them in one render command.
        s32 last_adjacent_index = i;
        for (s32 j = i; j < ui.draw_queue_size; ++j) {
            const auto &ui_cmd = ui.draw_queue[j];
            const auto &atlas  = ui.font_atlases[ui_cmd.atlas_index];

            if (ui_cmd.type != ui_draw_type || ui_cmd.atlas_index != atlas_index) {
                break;
            }
            
            last_adjacent_index = j;

            switch (ui_cmd.type) {
            case UI_DRAW_TEXT: {
                r_cmd.rid_vertex_array = tdb.rid_vertex_array;
                r_cmd.buffer_element_count = 4;
                r_cmd.instance_count += ui_cmd.instance_count;
                r_cmd.instance_offset = char_instance_offset;

                r_cmd.sid_material = tdb.sid_material;
                r_cmd.rid_override_texture = atlas.rid_texture;

                total_char_instance_offset += ui_cmd.instance_count;
            
                break;
            }
            case UI_DRAW_QUAD: {
                r_cmd.rid_vertex_array = qdb.rid_vertex_array;
                r_cmd.buffer_element_count += 4;
                r_cmd.buffer_element_offset = 4 * quad_instance_offset;

                // Instance count for quad should be 1 as for now vertex data for ui 
                // elements takes raw positions from vertex buffer unlike text that
                // takes transform matrix as vertex data and apply it to static quad
                // vertices.
                r_cmd.instance_count  = ui_cmd.instance_count;
                r_cmd.instance_offset = quad_instance_offset;

                r_cmd.sid_material = qdb.sid_material;
            
                total_quad_instance_offset += ui_cmd.instance_count;
            
                break;
            }
            }
        }
        
        i = last_adjacent_index;

        r_submit(&r_cmd);
    }

    ui.draw_queue_size = 0;

    tdb.char_count = 0;
    qdb.quad_count = 0;
}

u8 ui_button(uiid id, const char *text, const UI_Button_Style &style) {
    const auto &atlas = ui.font_atlases[style.atlas_index];    
    const s32 width = get_line_width_px(&atlas, text);
    const s32 height = atlas.line_height;
    const vec3 p0 = style.pos_text - vec3(style.padding.x, style.padding.y, style.pos_text.z);
    const vec3 p1 = vec3(p0.x + width + 2 * style.padding.x, p0.y + height + 2 * style.padding.y, p0.z);

    u8 flags = 0;
    u32 color_text = style.color_text.cold;
    u32 color_quad = style.color_quad.cold;

    if (inside(viewport.mouse_pos, p0.to_vec2(), p1.to_vec2())) {
        if (id != ui.id_hot) {
            flags |= UI_FLAG_HOT;
        }
        
        ui.id_hot = id;

        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;

        if (down_now(MOUSE_LEFT)) {
            ui.id_active = id;
            flags |= UI_FLAG_ACTIVATED;
        }
    } else {
        if (id == ui.id_hot) {
            ui.id_hot = UIID_NONE;
            flags |= UI_FLAG_UNHOT;
        }        
    }
    
    if (id == ui.id_active) {
        color_text = style.color_text.active;
        color_quad = style.color_quad.active;
            
        if (up_now(MOUSE_LEFT)) {
            if (id == ui.id_hot) {
                flags |= UI_FLAG_FINISHED;
            }

            ui.id_active = UIID_NONE;
            flags |= UI_FLAG_LOST;
        }
    }
    
    ui_quad(p0, p1, color_quad);
    ui_text(text, vec3(style.pos_text.x, style.pos_text.y, style.pos_text.z + F32_EPSILON), color_text, style.atlas_index);
    
    return flags;
}

u8 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style) {
    const auto &atlas = ui.font_atlases[style.atlas_index];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;
    const s32 width = atlas.space_advance_width * size;
    const s32 height = atlas.line_height;

    const vec3 p0 = style.pos_text - vec3(style.padding.x, style.padding.y, style.pos_text.z);
    const vec3 p1 = vec3(p0.x + width + 2 * style.padding.x,
                         p0.y + height + 2 * style.padding.y,
                         p0.z);

    u8 flags = 0;
    u32 color_text = style.color_text.cold;
    u32 color_quad = style.color_quad.cold;
    u32 color_cursor = style.color_cursor.cold;

    u32 count = (u32)str_size(text);
    
    if (inside(viewport.mouse_pos, p0.to_vec2(), p1.to_vec2())) {
        if (id != ui.id_hot) {
            flags |= UI_FLAG_HOT;
        }
        
        ui.id_hot = id;
        
        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;
        color_cursor = style.color_cursor.hot;

        if (down_now(MOUSE_LEFT)) {
            ui.id_active = id;
            flags |= UI_FLAG_ACTIVATED;
        }
    } else {
        if (id == ui.id_hot) {
            ui.id_hot = UIID_NONE;
            flags |= UI_FLAG_UNHOT;
        }
    }

    if (id == ui.id_active) {
        color_text = style.color_text.active;
        color_quad = style.color_quad.active;
        color_cursor = style.color_cursor.active;

        for (s32 i = 0; i < window_event_queue_size; ++i) {
            const auto &e = window_event_queue[i];
            if (e.type == WINDOW_EVENT_TEXT_INPUT) {
                if (count < size && is_ascii_printable(e.character)) {
                    text[count + 0] = e.character;
                    text[count + 1] = '\0';
                    count += 1;

                    flags |= UI_FLAG_CHANGED;
                }
                
                if (count > 0 && e.character == ASCII_BACKSPACE) {
                    text[count - 1] = '\0';
                    count -= 1;

                    flags |= UI_FLAG_CHANGED;
                }

                if (e.character == ASCII_NEW_LINE || e.character == ASCII_CARRIAGE_RETURN) {
                    ui.id_active = UIID_NONE;
                    flags |= UI_FLAG_FINISHED;
                }
            }
        }

        if (id != ui.id_hot && down_now(MOUSE_LEFT)) {
            ui.id_active = UIID_NONE;
            flags |= UI_FLAG_LOST;
        }
    }

    ui_quad(p0, p1, color_quad);
    ui_text(text, vec3(style.pos_text.x, style.pos_text.y, style.pos_text.z + F32_EPSILON), color_text, style.atlas_index);

    if (id == ui.id_active) {
        const f32 offset = (f32)atlas.space_advance_width * count;
        const vec3 cp0 = vec3(style.pos_text.x + offset, style.pos_text.y, style.pos_text.z + F32_EPSILON);
        const vec3 cp1 = vec3(cp0.x + 2.0f, cp0.y + ascent - descent, cp0.z);
    
        ui_quad(cp0, cp1, color_cursor);
    }
    
    return flags;
}

u8 ui_input_f32(uiid id, f32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_F32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_f32(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%.3f", *v);
    }
    
    return flags;
}

u8 ui_input_s8(uiid id, s8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_s8(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return flags;
}

u8 ui_input_s16(uiid id, s16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_s16(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return flags;
}

u8 ui_input_s32(uiid id, s32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_s32(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return flags;
}

u8 ui_input_s64(uiid id, s64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_s64(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%lld", *v);
    }
    
    return flags;
}

u8 ui_input_u8(uiid id, u8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_u8(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return flags;
}

u8 ui_input_u16(uiid id, u16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_u16(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return flags;
}

u8 ui_input_u32(uiid id, u32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_u32(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }

    return flags;
}

u8 ui_input_u64(uiid id, u64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = str_to_u64(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%llu", *v);
    }

    return flags;
}

u8 ui_input_sid(uiid id, sid *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_SID;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const u8 flags = ui_input_text(id, text, size, style);
    if (flags & UI_FLAG_FINISHED) {
        *v = sid_intern(text);
    }

    if (id != ui.id_active) {
        stbsp_snprintf(text, size, "%s", sid_str(*v));
    }

    return flags;
}

u8 ui_combo(uiid id, u32 *selected_index, const char **options, u32 option_count, const UI_Combo_Style &style) {
    Assert(*selected_index < option_count);

    u8 flags = 0;
    
    const UI_Button_Style switch_button_style = {
        style.pos_text,
        style.padding,
        style.color_text,
        style.color_quad,
        style.atlas_index,
    };

    const char *option = options[*selected_index];
    const u8 switch_button_flags = ui_button(id, option, switch_button_style);
    
    if (switch_button_flags & UI_FLAG_FINISHED) {
        flags |= UI_FLAG_CHANGED;
        flags |= UI_FLAG_FINISHED;
        
        *selected_index += 1;
        if (*selected_index >= option_count) {
            *selected_index = 0;
        }
    }

    return flags;
}

void ui_text(const char *text, vec3 pos, u32 color, s32 atlas_index) {
    ui_text(text, (u32)str_size(text), pos, color, atlas_index);
}

void ui_text(const char *text, u32 count, vec3 pos, u32 color, s32 atlas_index) {
    if (rgba_get_a(color) == 0) return;

    Assert(atlas_index < MAX_UI_FONT_ATLASES);
    Assert(ui.draw_queue_size < MAX_UI_DRAW_QUEUE_SIZE);

	const auto &atlas = ui.font_atlases[atlas_index];
    auto &tdb = ui.text_draw_buffer;

    auto &ui_cmd = ui.draw_queue[ui.draw_queue_size];
    ui_cmd.type = UI_DRAW_TEXT;
    ui_cmd.z = pos.z;
    ui_cmd.instance_count = 0;
    ui_cmd.atlas_index = atlas_index;
    
    ui.draw_queue_size += 1;
    
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < count; ++i) {
        Assert(tdb.char_count < MAX_UI_TEXT_DRAW_BUFFER_CHARS);
        
		const char c = text[i];

		if (c == ' ') {
			x += atlas.space_advance_width;
			continue;
		}

		if (c == '\t') {
			x += 4 * atlas.space_advance_width;
			continue;
		}

		if (c == '\n') {
			x = pos.x;
			y -= atlas.line_height;
			continue;
		}

		Assert((u32)c >= atlas.start_charcode);
		Assert((u32)c <= atlas.end_charcode);

		const u32 ci = c - atlas.start_charcode;
		const Font_Glyph_Metric *metric = atlas.metrics + ci;

		const f32 cw = (f32)atlas.font_size;
		const f32 ch = (f32)atlas.font_size;
		const f32 cx = x + metric->offset_x;
		const f32 cy = y - (ch + metric->offset_y);

        const vec3 location  = vec3(cx, cy, 0.0f);
        const vec3 scale     = vec3(cw, ch, 0.0f);
        const mat4 transform = mat4_identity().translate(location).scale(scale);

        tdb.colors[tdb.char_count] = color;
        tdb.charmap[tdb.char_count] = ci;
		tdb.transforms[tdb.char_count] = transform;
        tdb.char_count += 1;

        ui_cmd.instance_count += 1;
        
		x += metric->advance_width;
        if (i < count - 1) {
            x += atlas.px_h_scale * get_glyph_kern_advance(atlas.font, c, text[i + 1]);
        }
	}
}

void ui_text_with_shadow(const char *text, vec3 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index) {
    ui_text_with_shadow(text, (u32)str_size(text), pos, color, shadow_offset, shadow_color, atlas_index);
}

void ui_text_with_shadow(const char *text, u32 count, vec3 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index) {
	ui_text(text, count, pos + vec3(shadow_offset.x, shadow_offset.y, 0.0f), shadow_color, atlas_index);
	ui_text(text, count, pos, color, atlas_index);
}

void ui_quad(vec3 p0, vec3 p1, u32 color) {
    if (rgba_get_a(color) == 0) return;
    
    Assert(ui.draw_queue_size < MAX_UI_DRAW_QUEUE_SIZE);

    auto &ui_cmd = ui.draw_queue[ui.draw_queue_size];
    ui_cmd.type = UI_DRAW_QUAD;
    ui_cmd.z = Max(p0.z, p1.z);
    ui_cmd.instance_count = 1;

    ui.draw_queue_size += 1;

    auto &qdb = ui.quad_draw_buffer;

    const f32 x0 = Min(p0.x, p1.x);
    const f32 y0 = Min(p0.y, p1.y);

    const f32 x1 = Max(p0.x, p1.x);
    const f32 y1 = Max(p0.y, p1.y);
    
    vec2 *vp = qdb.positions + 4 * qdb.quad_count;
    u32  *vc = qdb.colors    + 4 * qdb.quad_count;

    vp[0] = vec2(x0, y0);
    vp[1] = vec2(x1, y0);
    vp[2] = vec2(x0, y1);    
    vp[3] = vec2(x1, y1);

    // @Cleanup: thats a bit unfortunate that we can't use instanced vertex advance rate
    // for color, maybe there is a clever solution somewhere.
    vc[0] = color;
    vc[1] = color;
    vc[2] = color;
    vc[3] = color;
    
    qdb.quad_count += 1;
}
