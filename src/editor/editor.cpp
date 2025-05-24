#include "pch.h"
#include "editor/hot_reload.h"
#include "editor/debug_console.h"

#include "os/time.h"
#include "os/file.h"
#include "os/input.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/render_command.h"
#include "render/render_registry.h"

#include "math/vector.h"
#include "math/matrix.h"

#include "log.h"
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
    debug_console.history_buffer = (char *)allocl(MAX_DEBUG_CONSOLE_BUFFER_SIZE);
    debug_console.text_to_draw = debug_console.history_buffer;
}

void open_debug_console() {
    if (debug_console.is_open) return;

    debug_console.is_open = true;
}

void close_debug_console() {
    if (!debug_console.is_open) return;

    debug_console.is_open = false;
}

void draw_debug_console() {
    if (debug_console.is_open) {
        const vec2 text_pos = vec2(100.0f);
        
        const vec2 quad_p0 = text_pos - vec2(debug_console.text_padding);
        const vec2 quad_p1 = vec2(viewport.width - text_pos.x, quad_p0.y + ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX]->font_size + 2 * debug_console.text_padding);
        const vec4 quad_color = vec4(0.0f, 0.0f, 0.0f, 0.6f);
        
        ui_draw_quad(quad_p0, quad_p1, quad_color);
        ui_draw_text(debug_console.text_input, debug_console.text_input_size, vec2(100.0f), vec3_red);
    }
}

void on_debug_console_text_input(u32 character) {
    if (!debug_console.is_open) return;

    //log("%u", character);
    
    if (character == ASCII_GRAVE_ACCENT) {
        return;
    }
    
    if (character == ASCII_NEW_LINE || character == ASCII_CARRIAGE_RETURN) {
        
        
        debug_console.text_input_size = 0;
        return;
    }

    if (character == ASCII_BACKSPACE) {
        debug_console.text_input_size -= 1;
        debug_console.text_input_size = Max(0, debug_console.text_input_size);
    }

    if (is_ascii_printable(character)) {
        if (debug_console.text_input_size >= MAX_DEBUG_CONSOLE_TEXT_INPUT_SIZE) {
            return;
        }
        
        debug_console.text_input[debug_console.text_input_size] = (char)character;
        debug_console.text_input_size += 1;
    }
}
