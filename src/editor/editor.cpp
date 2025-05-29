#include "pch.h"
#include "editor/hot_reload.h"
#include "editor/debug_console.h"

#include "os/time.h"
#include "os/file.h"
#include "os/input.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/render_stats.h"
#include "render/render_command.h"
#include "render/render_registry.h"

#include "math/math_core.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "log.h"
#include "str.h"
#include "profile.h"
#include "font.h"
#include "asset.h"

void register_hot_reload_directory(Hot_Reload_List *list, const char *path) {
	Assert(list->path_count < MAX_HOT_RELOAD_DIRECTORIES);
    list->paths[list->path_count] = path;
    list->path_count += 1;
	log("Registered hot reload directory %s", path);
}

static void hot_reload_file_callback(const File_Callback_Data *callback_data) {
    char relative_path[MAX_PATH_SIZE];
    convert_to_relative_asset_path(relative_path, callback_data->path);

    const auto sid = cache_sid(relative_path);
    if (Asset_Source *source = asset_source_table.find(sid)) {
        if (source->last_write_time != callback_data->last_write_time) {
            auto list = (Hot_Reload_List *)callback_data->user_data;

            Assert(list->reload_count < MAX_HOT_RELOAD_ASSETS);
            list->reload_sids[list->reload_count] = sid;
            list->reload_count += 1;
            
            source->last_write_time = callback_data->last_write_time;
        }
    }
}

void check_for_hot_reload(Hot_Reload_List *list) {
    PROFILE_SCOPE(__FUNCTION__);
    
    for (s32 i = 0; i < list->path_count; ++i) {
        for_each_file(list->paths[i], &hot_reload_file_callback, list);
    }

    for (s32 i = 0; i < list->reload_count; ++i) {
        START_SCOPE_TIMER(asset);
        Asset &asset = asset_table[list->reload_sids[i]];

        char path[MAX_PATH_SIZE];
        convert_to_full_asset_path(path, asset.relative_path);
            
        switch (asset.type) {
        case ASSET_SHADER: {
            char *data = (char *)allocl(MAX_SHADER_SIZE);
            defer { freel(MAX_SHADER_SIZE); };

            u64 bytes_read = 0;
            if (read_file(path, data, MAX_SHADER_SIZE, &bytes_read)) {
                data[bytes_read] = '\0';
    
                if (recreate_shader(asset.registry_index, data, asset.relative_path)) {
                    log("Hot reloaded shader %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(asset));
                } else {
                    error("Failed to hot reload shader %s, see errors above", path);
                }
            }
            
            break;
        }
        case ASSET_TEXTURE: {
            s32 try_count = 0;
            Texture_Memory memory;
            do {
                try_count += 1;
                memory = load_texture_memory(path, false);
            } while (!memory.data);
            
            const auto &texture = render_registry.textures[asset.registry_index];
            const auto format   = get_desired_texture_format(asset.as_texture.channel_count);
            
            if (recreate_texture(asset.registry_index, texture.type, texture.format, asset.as_texture.width, asset.as_texture.height, memory.data)) {
                if (texture.flags & TEXTURE_FLAG_HAS_MIPMAPS) {
                    generate_texture_mipmaps(asset.registry_index);
                }
                
                log("Hot reloaded texture %s in %.2fms after %d tries", path, CHECK_SCOPE_TIMER_MS(asset), try_count);
            } else {
                error("Failed to hot reload texture %s, see errors above", path);
            }
            
            free_texture_memory(&memory);
            break;
        }
        }
    }

    list->reload_count = 0;
}

void init_debug_console() {
    Assert(!debug_console.history);

    auto &history = debug_console.history;
    auto &history_y = debug_console.history_y;
    auto &history_min_y = debug_console.history_min_y;
    auto &history_max_width = debug_console.history_max_width;

    history = (char *)allocl(MAX_DEBUG_CONSOLE_HISTORY_SIZE);
    history[0] = '\0';

    on_debug_console_resize(viewport.width, viewport.height);
}

void open_debug_console() {
    if (debug_console.is_open) return;

    debug_console.is_open = true;
}

void close_debug_console() {
    if (!debug_console.is_open) return;

    debug_console.cursor_blink_dt = 0.0f;
    debug_console.is_open = false;
}

void draw_debug_console() {
    PROFILE_SCOPE(__FUNCTION__);
    
    if (debug_console.is_open) {
        auto &history = debug_console.history;
        auto &history_size = debug_console.history_size;
        auto &history_height = debug_console.history_height;
        auto &history_y = debug_console.history_y;
        auto &history_min_y = debug_console.history_min_y;
        auto &history_max_width = debug_console.history_max_width;
        auto &input = debug_console.input;
        auto &input_size = debug_console.input_size;
        auto &cursor_blink_dt = debug_console.cursor_blink_dt;
    
        const auto &atlas = *ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

        cursor_blink_dt += delta_time;
        
        const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
        const f32 descent = atlas.font->descent * atlas.px_h_scale;
        // @Cleanup: probably not ideal solution to get lower-case glyph height.
        const f32 lower_case_height = (atlas.font->ascent + atlas.font->descent) * atlas.px_h_scale;
            
        const vec2 it_pos = vec2(DEBUG_CONSOLE_MARGIN);
        const vec4 it_color = vec4_white;
        const vec2 its_offset = vec2(atlas.font_size * 0.1f, -atlas.font_size * 0.1f);
        const vec4 its_color = vec4_black;
        
        const vec2 iq_p0 = it_pos - vec2(DEBUG_CONSOLE_PADDING);
        const vec2 iq_p1 = vec2(viewport.width - DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING, iq_p0.y + lower_case_height + 2 * DEBUG_CONSOLE_PADDING);
        const vec4 iq_color = vec4(0.0f, 0.0f, 0.0f, 0.8f);

        const s32 input_width_px = get_line_width_px(&atlas, input, input_size);
        const vec2 cursor_p0 = vec2(it_pos.x + input_width_px + 1, it_pos.y + descent);
        const vec2 cursor_p1 = vec2(cursor_p0.x + atlas.space_advance_width, it_pos.y + ascent);
        vec4 cursor_color = it_color;

        if (cursor_blink_dt > DEBUG_CONSOLE_CURSOR_BLINK_INTERVAL) {
            if (cursor_blink_dt > 2 * DEBUG_CONSOLE_CURSOR_BLINK_INTERVAL) {
                cursor_blink_dt = 0.0f;
            } else {
                cursor_color.w = 0.0f;
            }
        }
        
        const vec2 ht_pos = vec2(DEBUG_CONSOLE_MARGIN, history_min_y);
        const vec4 ht_color = vec4(0.8f, 0.8f, 0.8f, 1.0f);
        const vec2 hts_offset = vec2(atlas.font_size * 0.1f, -atlas.font_size * 0.1f);
        const vec4 hts_color = vec4_black;

        const vec2 hq_p0 = vec2(iq_p0.x, iq_p1.y + DEBUG_CONSOLE_PADDING);
        const vec2 hq_p1 = vec2(viewport.width - DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING, ht_pos.y + ascent + DEBUG_CONSOLE_PADDING);
        const vec4 hq_color = vec4(0.0f, 0.0f, 0.0f, 0.8f);
        const f32 hq_height = absf(hq_p0.y - hq_p1.y);

        const f32 max_history_height = hq_height - DEBUG_CONSOLE_PADDING;

        history_height = 0.0f;
        char *history_start = history;
        f32 history_visible_height = history_size > 0 ? atlas.line_height : 0.0f;
        f32 history_pointer_y = history_y;
        s32 history_draw_count = 0;
        
        for (s32 i = 0; i < history_size; ++i) {
            if (history_pointer_y > history_min_y) {
                history_start += 1;
                history_visible_height = 0.0f;
            } else {
                history_draw_count += 1;
            }
            
            if (history[i] == ASCII_NEW_LINE) {
                history_height += atlas.line_height;
                history_visible_height += atlas.line_height;
                history_pointer_y -= atlas.line_height;
                
                if (history_visible_height > max_history_height) {
                    break;
                }
            }
        }

        history_draw_count = Min(history_draw_count, MAX_DEBUG_CONSOLE_HISTORY_SIZE);
        
        ui_draw_quad(iq_p0, iq_p1, iq_color);
        ui_draw_quad(hq_p0, hq_p1, hq_color);
        
        ui_draw_text(input, input_size, it_pos, it_color, UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX);
        ui_draw_text(history_start, history_draw_count, ht_pos, ht_color, UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX);

        ui_draw_quad(cursor_p0, cursor_p1, cursor_color);
    }
}

void add_to_debug_console_history(const char *text, u32 count) {
    if (!debug_console.history) {
        return;
    }

    auto &history = debug_console.history;
    auto &history_size = debug_console.history_size;
    auto &history_max_width = debug_console.history_max_width;

    const auto &atlas = *ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

    f32 text_width = 0.0f; 
    for (u32 i = 0; i < count; ++i) {
        const char c = text[i];

        // @Cleanup: make better history overflow handling.
        if (history_size > MAX_DEBUG_CONSOLE_HISTORY_SIZE) {
            history[0] = '\0';
            history_size = 0;
        }
   
        if (text_width < history_max_width) {            
            text_width += get_char_width_px(&atlas, c);
            if (c == ASCII_NEW_LINE) {
                text_width = 0;
            }
        } else { // wrap lines that do not fit into debug console window
            history[history_size] = ASCII_NEW_LINE;
            history_size += 1;

            text_width = 0;
        }

        history[history_size] = c;
        history_size += 1; 
    }

    history[history_size] = '\0';
}

void on_debug_console_input(u32 character) {
    if (!debug_console.is_open) return;
    
    if (character == ASCII_GRAVE_ACCENT) {
        return;
    }

    auto &history = debug_console.history;
    auto &history_size = debug_console.history_size;
    auto &history_y = debug_console.history_y;
    auto &history_min_y = debug_console.history_min_y;
    auto &input = debug_console.input;
    auto &input_size = debug_console.input_size;
    auto &cursor_blink_dt = debug_console.cursor_blink_dt;

    const auto &atlas = *ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

    cursor_blink_dt = 0.0f;
    
    if (character == ASCII_NEW_LINE || character == ASCII_CARRIAGE_RETURN) {
        if (input_size > 0) {
            static char add_text[MAX_DEBUG_CONSOLE_INPUT_SIZE + 128] = { '\0' };
            
            // @Cleanup: make better history overflow handling.
            if (history_size + input_size > MAX_DEBUG_CONSOLE_HISTORY_SIZE) {
                history[0] = '\0';
                history_size = 0;
            }

            const bool clear = str_cmp(input, DEBUG_CONSOLE_COMMAND_CLEAR, input_size);
            if (clear) {
                history[0] = '\0';
                history_size = 0;
                history_y = history_min_y;
            } else {
                static const u32 warning_size = (u32)str_size(DEBUG_CONSOLE_UNKNOWN_COMMAND_WARNING);
                str_glue(add_text, DEBUG_CONSOLE_UNKNOWN_COMMAND_WARNING, warning_size);
            }

            if (!clear) {
                str_glue(add_text, input, input_size);
                str_glue(add_text, "\n",  1);
                add_to_debug_console_history(add_text, (u32)str_size(add_text));
            }
            
            input_size = 0;
            add_text[0] = '\0';
        }
        
        return;
    }

    if (character == ASCII_BACKSPACE) {
        input_size -= 1;
        input_size = Max(0, input_size);
    }

    if (is_ascii_printable(character)) {
        if (input_size >= MAX_DEBUG_CONSOLE_INPUT_SIZE) {
            return;
        }
        
        input[input_size] = (char)character;
        input_size += 1;
    }
}

void on_debug_console_scroll(s32 delta) {
    auto &history_height = debug_console.history_height;
    auto &history_y = debug_console.history_y;
    auto &history_min_y = debug_console.history_min_y;

    const auto &atlas = *ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

    history_y -= delta * atlas.line_height;
    history_y = Clamp(history_y, history_min_y, history_min_y + history_height);
}

void on_debug_console_resize(s16 width, s16 height) {
    auto &history_y = debug_console.history_y;
    auto &history_min_y = debug_console.history_min_y;
    auto &history_max_width = debug_console.history_max_width;

    history_min_y = height - DEBUG_CONSOLE_MARGIN;
    history_y = history_min_y;

    history_max_width = width - 2 * DEBUG_CONSOLE_MARGIN;
}
