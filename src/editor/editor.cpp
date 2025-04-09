#include "pch.h"
#include "editor/hot_reload.h"

#include "os/time.h"
#include "os/file.h"

#include "render/render_registry.h"

#include "log.h"
#include "profile.h"
#include "asset_registry.h"

void register_hot_reload_directory(Hot_Reload_List *list, const char *path) {
	assert(list->path_count < MAX_HOT_RELOAD_DIRECTORIES);
    list->paths[list->path_count++] = path;
	log("Registered hot reload directory %s", path);
}

static void hot_reload_file_callback(const File_Callback_Data *callback_data) {
    char relative_path[MAX_PATH_SIZE];
    convert_to_relative_asset_path(callback_data->path, relative_path);

    const auto sid = cache_sid(relative_path);
    if (Asset_Source *source = asset_source_table.find(sid)) {
        if (source->last_write_time != callback_data->last_write_time) {
            auto list = (Hot_Reload_List *)callback_data->user_data;
            assert(list->reload_count < MAX_HOT_RELOAD_ASSETS);
            list->reload_sids[list->reload_count++] = sid;
            source->last_write_time = callback_data->last_write_time;
        }
    }
}

void check_for_hot_reload(Hot_Reload_List *list) {    
    for (s32 i = 0; i < list->path_count; ++i) {
        for_each_file(list->paths[i], &hot_reload_file_callback, list);
    }

    for (s32 i = 0; i < list->reload_count; ++i) {
        START_SCOPE_TIMER(asset);
        Asset &asset = asset_table[list->reload_sids[i]];

        char path[MAX_PATH_SIZE];
        convert_to_full_asset_path(asset.relative_path, path);
            
        switch (asset.type) {
        case ASSET_SHADER: {
            char *data = push_array(temp, MAX_SHADER_SIZE, char);
            defer { pop(temp, MAX_SHADER_SIZE); };

            u64 bytes_read = 0;
            if (read_file(path, data, MAX_SHADER_SIZE, &bytes_read)) {
                data[bytes_read] = '\0';
    
                if (recreate_shader(asset.registry_index, data)) {
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
                try_count++;
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
