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
#include "camera.h"
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

bool operator==(const uiid &a, const uiid &b) {
    return a.owner == b.owner && a.item == b.item && a.index == b.index;
}

bool operator!=(const uiid &a, const uiid &b) {
    return !(a == b);
}

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

    {
        auto &lrender = R_ui.line_render;

        constexpr u32 pos_buffer_size   = 2 * lrender.MAX_LINES * sizeof(vec2);
        constexpr u32 color_buffer_size = 2 * lrender.MAX_LINES * sizeof(u32);

        const u32 pos_buffer_offset   = R_vertex_map_range.offset + R_vertex_map_range.size;
        const u32 color_buffer_offset = pos_buffer_offset + pos_buffer_size;
        
        lrender.positions = (vec2 *)r_alloc(R_vertex_map_range, pos_buffer_size).data;
        lrender.colors    = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
        
        R_Vertex_Binding bindings[2] = {};
        bindings[0].binding_index = 0;
        bindings[0].offset = pos_buffer_offset;
        bindings[0].component_count = 1;
        bindings[0].components[0] = { R_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].offset = color_buffer_offset;
        bindings[1].component_count = 1;
        bindings[1].components[0] = { R_U32, 0 };
        
        lrender.vertex_desc = r_create_vertex_descriptor(COUNT(bindings), bindings);;
        lrender.material = find_asset(SID_MATERIAL_UI_ELEMENT)->index;
    }
        
    {
        auto &qrender = R_ui.quad_render;

        constexpr u32 pos_buffer_size   = 4 * qrender.MAX_QUADS * sizeof(vec2);
        constexpr u32 color_buffer_size = 4 * qrender.MAX_QUADS * sizeof(u32);

        const u32 pos_buffer_offset   = R_vertex_map_range.offset + R_vertex_map_range.size;
        const u32 color_buffer_offset = pos_buffer_offset + pos_buffer_size;
        
        qrender.positions = (vec2 *)r_alloc(R_vertex_map_range, pos_buffer_size).data;
        qrender.colors    = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
        
        R_Vertex_Binding bindings[2] = {};
        bindings[0].binding_index = 0;
        bindings[0].offset = pos_buffer_offset;
        bindings[0].component_count = 1;
        bindings[0].components[0] = { R_F32_2, 0 };

        bindings[1].binding_index = 1;
        bindings[1].offset = color_buffer_offset;
        bindings[1].component_count = 1;
        bindings[1].components[0] = { R_U32, 0 };
        
        qrender.vertex_desc = r_create_vertex_descriptor(COUNT(bindings), bindings);;
        qrender.material = find_asset(SID_MATERIAL_UI_ELEMENT)->index;
    }

    {
        auto &trender = R_ui.text_render;
        
        constexpr f32 vertices[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };

        constexpr u32 position_buffer_size  = sizeof(vertices);
        constexpr u32 color_buffer_size     = trender.MAX_CHARS * sizeof(u32);
        constexpr u32 charmap_buffer_size   = trender.MAX_CHARS * sizeof(u32);
        constexpr u32 transform_buffer_size = trender.MAX_CHARS * sizeof(mat4);

        const u32 position_buffer_offset  = R_vertex_map_range.offset + R_vertex_map_range.size;
        const u32 color_buffer_offset     = position_buffer_offset + position_buffer_size;
        const u32 charmap_buffer_offset   = color_buffer_offset + color_buffer_size;
        const u32 transform_buffer_offset = charmap_buffer_offset + charmap_buffer_size;

        trender.positions  = (f32  *)r_alloc(R_vertex_map_range, position_buffer_size).data;
        trender.colors     = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
        trender.charmap    = (u32  *)r_alloc(R_vertex_map_range, charmap_buffer_size).data;
        trender.transforms = (mat4 *)r_alloc(R_vertex_map_range, transform_buffer_size).data;

        mem_copy(trender.positions, vertices, position_buffer_size);
        
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
        
        trender.vertex_desc = r_create_vertex_descriptor(COUNT(bindings), bindings);;
        trender.material = find_asset(SID_MATERIAL_UI_TEXT)->index;
    }
}

void ui_flush() {
    PROFILE_SCOPE(__FUNCTION__);
    
    r_submit(R_ui.command_list);

    auto &lrender = R_ui.line_render;
    auto &qrender = R_ui.quad_render;
    auto &trender = R_ui.text_render;
    
    lrender.line_count = 0;
    qrender.quad_count = 0;
    trender.char_count = 0;
}

u16 ui_button(uiid id, const char *text, const UI_Button_Style &style) {
    const auto &atlas = R_ui.font_atlases[style.atlas_index];    
    const s32 width = get_line_width_px(atlas, text);
    const s32 height = atlas.line_height;
    const vec2 p0 = style.pos_text - style.padding;
    const vec2 p1 = vec2(p0.x + width + 2 * style.padding.x, p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    u32 color_text = style.color_text.cold;
    u32 color_quad = style.color_quad.cold;

    if (inside(R_viewport.mouse_pos, p0, p1)) {
        if (id != R_ui.id_hot) {
            bits |= UI_HOT_BIT;
        }
        
        R_ui.id_hot = id;

        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;

        if (down_now(MOUSE_LEFT)) {
            R_ui.id_active = id;
            bits |= UI_ACTIVATED_BIT;
        }
    } else {
        if (id == R_ui.id_hot) {
            R_ui.id_hot = UIID_NONE;
            bits |= UI_UNHOT_BIT;
        }        
    }
    
    if (id == R_ui.id_active) {
        color_text = style.color_text.active;
        color_quad = style.color_quad.active;
            
        if (up_now(MOUSE_LEFT)) {
            if (id == R_ui.id_hot) {
                bits |= UI_FINISHED_BIT;
            }

            R_ui.id_active = UIID_NONE;
            bits |= UI_LOST_BIT;
        }
    }
    
    ui_quad(p0, p1, color_quad, style.z);
    ui_text(text, style.pos_text, color_text, style.z + F32_EPSILON, style.atlas_index);
    
    return bits;
}

u16 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style) {
    const auto &atlas = R_ui.font_atlases[style.atlas_index];
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;
    const s32 width = atlas.space_advance_width * size;
    const s32 height = atlas.line_height;

    const vec2 p0 = style.pos_text - style.padding;
    const vec2 p1 = vec2(p0.x + width  + 2 * style.padding.x,
                         p0.y + height + 2 * style.padding.y);

    u16 bits = 0;
    u32 color_text = style.color_text.cold;
    u32 color_quad = style.color_quad.cold;
    u32 color_cursor = style.color_cursor.cold;

    u32 count = (u32)str_size(text);
    
    if (inside(R_viewport.mouse_pos, p0, p1)) {
        if (id != R_ui.id_hot) {
            bits |= UI_HOT_BIT;
        }
        
        R_ui.id_hot = id;
        
        color_text = style.color_text.hot;
        color_quad = style.color_quad.hot;
        color_cursor = style.color_cursor.hot;

        if (down_now(MOUSE_LEFT)) {
            R_ui.id_active = id;
            bits |= UI_ACTIVATED_BIT;
        }
    } else {
        if (id == R_ui.id_hot) {
            R_ui.id_hot = UIID_NONE;
            bits |= UI_UNHOT_BIT;
        }
    }

    if (id == R_ui.id_active) {
        color_text = style.color_text.active;
        color_quad = style.color_quad.active;
        color_cursor = style.color_cursor.active;

        for (s32 i = 0; i < Main_window.event_count; ++i) {
            const auto &e = Main_window.events[i];
            if (e.type == WINDOW_EVENT_TEXT_INPUT) {
                if (count < size && is_ascii_printable(e.character)) {
                    text[count + 0] = e.character;
                    text[count + 1] = '\0';
                    count += 1;

                    bits |= UI_CHANGED_BIT;
                }
                
                if (count > 0 && e.character == ASCII_BACKSPACE) {
                    text[count - 1] = '\0';
                    count -= 1;

                    bits |= UI_CHANGED_BIT;
                }

                if (e.character == ASCII_NEW_LINE || e.character == ASCII_CARRIAGE_RETURN) {
                    R_ui.id_active = UIID_NONE;
                    bits |= UI_FINISHED_BIT;
                }
            }
        }

        if (id != R_ui.id_hot && down_now(MOUSE_LEFT)) {
            R_ui.id_active = UIID_NONE;
            bits |= UI_LOST_BIT;
        }
    }

    ui_quad(p0, p1, color_quad, style.z);
    ui_text(text, style.pos_text, color_text, style.z + F32_EPSILON, style.atlas_index);

    if (id == R_ui.id_active) {
        const f32 offset = (f32)atlas.space_advance_width * count;
        const vec2 p0 = vec2(style.pos_text.x + offset, style.pos_text.y);
        const vec2 p1 = vec2(p0.x + 2.0f, p0.y + ascent - descent);
    
        ui_quad(p0, p1, color_cursor, style.z + F32_EPSILON);
    }
    
    return bits;
}

u16 ui_input_f32(uiid id, f32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_F32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_f32(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%.3f", *v);
    }
    
    return bits;
}

u16 ui_input_s8(uiid id, s8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_s8(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s16(uiid id, s16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_s16(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s32(uiid id, s32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_s32(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%d", *v);
    }
    
    return bits;
}

u16 ui_input_s64(uiid id, s64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_S64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_s64(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%lld", *v);
    }
    
    return bits;
}

u16 ui_input_u8(uiid id, u8 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U8;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_u8(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u16(uiid id, u16 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U16;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_u16(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }
    
    return bits;
}

u16 ui_input_u32(uiid id, u32 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U32;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_u32(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%u", *v);
    }

    return bits;
}

u16 ui_input_u64(uiid id, u64 *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_U64;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = str_to_u64(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%llu", *v);
    }

    return bits;
}

u16 ui_input_sid(uiid id, sid *v, const UI_Input_Style &style) {
    constexpr u32 size = UI_INPUT_BUFFER_SIZE_SID;
    
    char *text = get_or_alloc_input_buffer(id, size);
    
    const auto bits = ui_input_text(id, text, size, style);
    if (bits & UI_FINISHED_BIT) {
        *v = sid_intern(text);
    }

    if (id != R_ui.id_active) {
        stbsp_snprintf(text, size, "%s", sid_str(*v));
    }

    return bits;
}

u16 ui_combo(uiid id, u32 *selected_index, const char **options, u32 option_count, const UI_Combo_Style &style) {
    Assert(*selected_index < option_count);

    u16 bits = 0;
    
    const UI_Button_Style button_style = {
        style.z,
        style.pos_text,
        style.padding,
        style.color_text,
        style.color_quad,
        style.atlas_index,
    };

    const char *option = options[*selected_index];
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

void ui_text(const char *text, vec2 pos, u32 color, f32 z, s32 atlas_index) {
    ui_text(text, (u32)str_size(text), pos, color, z, atlas_index);
}

void ui_text(const char *text, u32 count, vec2 pos, u32 color, f32 z, s32 atlas_index) {
    if (rgba_get_a(color) == 0) return;

    Assert(atlas_index < R_ui.MAX_FONT_ATLASES);
    
    auto &trender = R_ui.text_render;
    Assert(trender.char_count + count <= trender.MAX_CHARS);

	const auto &atlas = R_ui.font_atlases[atlas_index];
    const auto &mt = R_table.materials[trender.material];
    
    R_Command cmd;
    cmd.mode = R_TRIANGLE_STRIP;
    cmd.shader = mt.shader;
    cmd.texture = atlas.texture;
    cmd.vertex_desc = trender.vertex_desc;
    cmd.first = 0;
    cmd.count = 4;
    cmd.base_instance = trender.char_count;
    cmd.instance_count = 0;
    cmd.uniforms = mt.uniforms;
    cmd.uniform_count = mt.uniform_count;
    
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < count; ++i) {
        Assert(trender.char_count < trender.MAX_CHARS);
        
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

        trender.colors[trender.char_count] = color;
        trender.charmap[trender.char_count] = ci;
		trender.transforms[trender.char_count] = transform;
        trender.char_count += 1;

        cmd.instance_count += 1;
        
		x += metric->advance_width;
        if (i < count - 1) {
            x += atlas.px_h_scale * get_glyph_kern_advance(*atlas.font, c, text[i + 1]);
        }
	}
    
    r_add(R_ui.command_list, cmd, ui_sort_key(z));
}

void ui_text_with_shadow(const char *text, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, f32 z, s32 atlas_index) {
    ui_text_with_shadow(text, (u32)str_size(text), pos, color, shadow_offset, shadow_color, z, atlas_index);
}

void ui_text_with_shadow(const char *text, u32 count, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, f32 z, s32 atlas_index) {
	ui_text(text, count, pos + shadow_offset, shadow_color, z, atlas_index);
	ui_text(text, count, pos, color, z + F32_EPSILON, atlas_index);
}

void ui_quad(vec2 p0, vec2 p1, u32 color, f32 z) {
    if (rgba_get_a(color) == 0) return;

    auto &qrender = R_ui.quad_render;
    Assert(qrender.quad_count < qrender.MAX_QUADS);

    const auto &mt = R_table.materials[qrender.material];
    
    R_Command cmd;
    cmd.mode = R_TRIANGLE_STRIP;
    cmd.shader = mt.shader;
    cmd.vertex_desc = qrender.vertex_desc;
    cmd.first = 4 * qrender.quad_count;
    cmd.count = 4;
    cmd.base_instance = qrender.quad_count;
    cmd.instance_count = 1;
    cmd.uniforms = mt.uniforms;
    cmd.uniform_count = mt.uniform_count;
    
    r_add(R_ui.command_list, cmd, ui_sort_key(z));

    const f32 x0 = Min(p0.x, p1.x);
    const f32 y0 = Min(p0.y, p1.y);

    const f32 x1 = Max(p0.x, p1.x);
    const f32 y1 = Max(p0.y, p1.y);
    
    vec2 *vp = qrender.positions + 4 * qrender.quad_count;
    u32  *vc = qrender.colors    + 4 * qrender.quad_count;

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
    
    qrender.quad_count += 1;
}

void ui_line(vec2 start, vec2 end, u32 color, f32 z) {
    auto &lrender = R_ui.line_render;
    Assert(lrender.line_count < lrender.MAX_LINES);

    const auto &mt = R_table.materials[lrender.material];
    
    R_Command cmd;
    cmd.mode = R_LINES;
    cmd.shader = mt.shader;
    cmd.vertex_desc = lrender.vertex_desc;
    cmd.first = 2 * lrender.line_count;
    cmd.count = 2;
    cmd.base_instance = lrender.line_count;
    cmd.instance_count = 1;
    cmd.uniforms = mt.uniforms;
    cmd.uniform_count = mt.uniform_count;
    
    r_add(R_ui.command_list, cmd, ui_sort_key(z));
    
    vec2 *vp = lrender.positions + 2 * lrender.line_count;
    u32  *vc = lrender.colors    + 2 * lrender.line_count;
        
    vp[0] = start;
    vp[1] = end;
    
    vc[0] = color;
    vc[1] = color;

    lrender.line_count += 1;
}

extern Rect to_rect(const R_Viewport &vp);

void ui_world_line(vec3 start, vec3 end, u32 color, f32 z) {
    const auto &camera = active_camera(World);
    const auto rect = to_rect(R_viewport);
    
    const vec2 vpstart = world_to_screen(rect, camera.view_proj, start);
    const vec2 vpend   = world_to_screen(rect, camera.view_proj, end);
    
    ui_line(vpstart, vpend, color, z);
}
