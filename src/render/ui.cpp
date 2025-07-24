#include "render/ui.h"
#include "render/r_viewport.h"
#include "render/r_table.h"
#include "render/r_storage.h"
#include "render/r_shader.h"
#include "render/r_material.h"
#include "render/r_vertex_descriptor.h"

#include "os/input.h"
#include "os/window.h"

#include "game/world.h"

#include "math/vector.h"
#include "math/matrix.h"

#include "str.h"
#include "hash.h"
#include "asset.h"
#include "profile.h"
#include "collision.h"
#include "reflection.h"
#include "hash_table.h"

#include "stb_sprintf.h"

constexpr u32 MAX_UI_INPUT_TABLE_SIZE  = 512;
constexpr u32 MAX_UI_INPUT_BUFFER_SIZE = KB(16);

typedef Hash_Table<uiid, char *> UI_Input_Table;

static UI_Input_Table ui_input_table = {};
static char *ui_input_buffer = null; // storage for input buffers
static u32 ui_input_buffer_size = 0;

static R_Sort_Key ui_sort_key(f32 z) {
    Assert(z <= UI_MAX_Z);
    
    R_Sort_Key sort_key;
        
    const auto &camera = active_camera(World);
    const f32 norm = z / UI_MAX_Z;

    Assert(norm >= 0.0f);
    Assert(norm <= 1.0f);
        
    const u32 bits = *(u32 *)&norm;
    sort_key.depth = bits >> 8;

    return sort_key;
}

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

    ui_input_buffer = (char *)allocp(MAX_UI_INPUT_BUFFER_SIZE);

    r_create_command_list(R_ui.MAX_COMMANDS, R_ui.command_list);
    
    {
        const auto &fna = *find_asset(SID_FONT_BETTER_VCR);
        const auto &ui_default_font = Font_infos[fna.index];

        Font_Atlas atlas_16, atlas_24;
        bake_font_atlas(ui_default_font, 33, 126, 16, atlas_16);
        bake_font_atlas(ui_default_font, 33, 126, 24, atlas_24);
        
        R_ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX]       = atlas_16;
        R_ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX] = atlas_16;
        R_ui.font_atlases[UI_PROFILER_FONT_ATLAS_INDEX]      = atlas_16;
        R_ui.font_atlases[UI_SCREEN_REPORT_FONT_ATLAS_INDEX] = atlas_24;
    }
    
    {   // Text draw buffer.
        auto &tdb = R_ui.text_draw_buffer;
        
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        constexpr u32 position_buffer_size  = sizeof(vertices);
        constexpr u32 color_buffer_size     = tdb.MAX_CHARS * sizeof(u32);
        constexpr u32 charmap_buffer_size   = tdb.MAX_CHARS * sizeof(u32);
        constexpr u32 transform_buffer_size = tdb.MAX_CHARS * sizeof(mat4);

        const u32 position_buffer_offset  = R_vertex_map_range.offset + R_vertex_map_range.size;
        const u32 color_buffer_offset     = position_buffer_offset + position_buffer_size;
        const u32 charmap_buffer_offset   = color_buffer_offset + color_buffer_size;
        const u32 transform_buffer_offset = charmap_buffer_offset + charmap_buffer_size;

        tdb.positions  = (f32  *)r_alloc(R_vertex_map_range, position_buffer_size).data;
        tdb.colors     = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
        tdb.charmap    = (u32  *)r_alloc(R_vertex_map_range, charmap_buffer_size).data;
        tdb.transforms = (mat4 *)r_alloc(R_vertex_map_range, transform_buffer_size).data;

        mem_copy(tdb.positions, vertices, position_buffer_size);
        
        R_Vertex_Binding bindings[4] = {};
        bindings[0].binding_index = 0;
        bindings[0].offset = position_buffer_offset;
        bindings[0].component_count = 1;
        bindings[0].components[0] = { R_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].offset = color_buffer_offset;
        bindings[1].component_count = 1;
        bindings[1].components[0] = { R_U32, 1 };

        bindings[2].binding_index = 2;
        bindings[2].offset = charmap_buffer_offset;
        bindings[2].component_count = 1;
        bindings[2].components[0] = { R_U32, 1 };

        bindings[3].binding_index = 3;
        bindings[3].offset = transform_buffer_offset;
        bindings[3].component_count = 4;
        bindings[3].components[0] = { R_F32_4, 1 };
        bindings[3].components[1] = { R_F32_4, 1 };
        bindings[3].components[2] = { R_F32_4, 1 };
        bindings[3].components[3] = { R_F32_4, 1 };
        
        tdb.vertex_desc = r_create_vertex_descriptor(COUNT(bindings), bindings);;
        tdb.material = find_asset(SID_MATERIAL_UI_TEXT)->index;
    }

    {   // Quad draw buffer.
        auto &qdb = R_ui.quad_draw_buffer;

        constexpr u32 pos_buffer_size   = 4 * qdb.MAX_QUADS * sizeof(vec2);
        constexpr u32 color_buffer_size = 1 * qdb.MAX_QUADS * sizeof(u32);

        const u32 pos_buffer_offset   = R_vertex_map_range.offset + R_vertex_map_range.size;
        const u32 color_buffer_offset = pos_buffer_offset + pos_buffer_size;
        
        qdb.positions = (vec2 *)r_alloc(R_vertex_map_range, pos_buffer_size).data;
        qdb.colors    = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
        
        R_Vertex_Binding bindings[2] = {};
        bindings[0].binding_index = 0;
        bindings[0].offset = pos_buffer_offset;
        bindings[0].component_count = 1;
        bindings[0].components[0] = { R_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].offset = color_buffer_offset;
        bindings[1].component_count = 1;
        bindings[1].components[0] = { R_U32, 0 };
        
        qdb.vertex_desc = r_create_vertex_descriptor(COUNT(bindings), bindings);;
        qdb.material = find_asset(SID_MATERIAL_UI_ELEMENT)->index;
    }
}

void ui_flush() {
    PROFILE_SCOPE(__FUNCTION__);
    
    r_submit(R_ui.command_list);

    auto &tdb = R_ui.text_draw_buffer;
    auto &qdb = R_ui.quad_draw_buffer;
    
    tdb.char_count = 0;
    qdb.quad_count = 0;
}

u8 ui_button(uiid id, const char *text, const UI_Button_Style &style) {
    const auto &atlas = R_ui.font_atlases[style.atlas_index];    
    const s32 width = get_line_width_px(atlas, text);
    const s32 height = atlas.line_height;
    const vec3 p0 = style.pos_text - vec3(style.padding.x, style.padding.y, style.pos_text.z);
    const vec3 p1 = vec3(p0.x + width + 2 * style.padding.x, p0.y + height + 2 * style.padding.y, p0.z);

    u8 flags = 0;
    u32 color_text = style.color_text.cold;
    u32 color_quad = style.color_quad.cold;

    if (inside(R_viewport.mouse_pos, p0.to_vec2(), p1.to_vec2())) {
        if (id != R_ui.id_hot) {
            flags |= UI_FLAG_HOT;
        }
        
        R_ui.id_hot = id;

        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;

        if (down_now(MOUSE_LEFT)) {
            R_ui.id_active = id;
            flags |= UI_FLAG_ACTIVATED;
        }
    } else {
        if (id == R_ui.id_hot) {
            R_ui.id_hot = UIID_NONE;
            flags |= UI_FLAG_UNHOT;
        }        
    }
    
    if (id == R_ui.id_active) {
        color_text = style.color_text.active;
        color_quad = style.color_quad.active;
            
        if (up_now(MOUSE_LEFT)) {
            if (id == R_ui.id_hot) {
                flags |= UI_FLAG_FINISHED;
            }

            R_ui.id_active = UIID_NONE;
            flags |= UI_FLAG_LOST;
        }
    }
    
    ui_quad(p0, p1, color_quad);
    ui_text(text, vec3(style.pos_text.x, style.pos_text.y, style.pos_text.z + F32_EPSILON), color_text, style.atlas_index);
    
    return flags;
}

u8 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style) {
    const auto &atlas = R_ui.font_atlases[style.atlas_index];
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
    
    if (inside(R_viewport.mouse_pos, p0.to_vec2(), p1.to_vec2())) {
        if (id != R_ui.id_hot) {
            flags |= UI_FLAG_HOT;
        }
        
        R_ui.id_hot = id;
        
        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;
        color_cursor = style.color_cursor.hot;

        if (down_now(MOUSE_LEFT)) {
            R_ui.id_active = id;
            flags |= UI_FLAG_ACTIVATED;
        }
    } else {
        if (id == R_ui.id_hot) {
            R_ui.id_hot = UIID_NONE;
            flags |= UI_FLAG_UNHOT;
        }
    }

    if (id == R_ui.id_active) {
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
                    R_ui.id_active = UIID_NONE;
                    flags |= UI_FLAG_FINISHED;
                }
            }
        }

        if (id != R_ui.id_hot && down_now(MOUSE_LEFT)) {
            R_ui.id_active = UIID_NONE;
            flags |= UI_FLAG_LOST;
        }
    }

    ui_quad(p0, p1, color_quad);
    ui_text(text, vec3(style.pos_text.x, style.pos_text.y, style.pos_text.z + F32_EPSILON), color_text, style.atlas_index);

    if (id == R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    if (id != R_ui.id_active) {
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

    Assert(atlas_index < R_ui.MAX_FONT_ATLASES);
    
    auto &tdb = R_ui.text_draw_buffer;

	const auto &atlas = R_ui.font_atlases[atlas_index];
    const auto &mt = R_table.materials[tdb.material];
    
    R_Command cmd;
    cmd.mode = R_TRIANGLE_STRIP;
    cmd.shader = mt.shader;
    cmd.texture = atlas.texture;
    cmd.vertex_desc = tdb.vertex_desc;
    cmd.first = 0;
    cmd.count = 4;
    cmd.base_instance = tdb.char_count;
    cmd.instance_count = 0;
    cmd.uniforms = mt.uniforms;
    cmd.uniform_count = mt.uniform_count;
    
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < count; ++i) {
        Assert(tdb.char_count < tdb.MAX_CHARS);
        
		const char c = text[i];

		if (c == ASCII_SPACE) {
			x += atlas.space_advance_width;
			continue;
		}

		if (c == ASCII_TAB) {
			x += 4 * atlas.space_advance_width;
			continue;
		}

		if (c == ASCII_NEW_LINE) {
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

        cmd.instance_count += 1;
        
		x += metric->advance_width;
        if (i < count - 1) {
            x += atlas.px_h_scale * get_glyph_kern_advance(*atlas.font, c, text[i + 1]);
        }
	}
    
    r_add(R_ui.command_list, cmd, ui_sort_key(pos.z));
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

    auto &qdb = R_ui.quad_draw_buffer;

    const auto &mt = R_table.materials[qdb.material];
    
    R_Command cmd;
    cmd.mode = R_TRIANGLE_STRIP;
    cmd.shader = mt.shader;
    cmd.vertex_desc = qdb.vertex_desc;
    cmd.first = 4 * qdb.quad_count;
    cmd.count = 4;
    cmd.base_instance = qdb.quad_count;
    cmd.instance_count = 1;
    cmd.uniforms = mt.uniforms;
    cmd.uniform_count = mt.uniform_count;
    
    r_add(R_ui.command_list, cmd, ui_sort_key(p0.z));

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
