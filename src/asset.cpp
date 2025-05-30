#include "pch.h"
#include "asset.h"
#include "log.h"
#include "sid.h"
#include "str.h"
#include "profile.h"
#include "font.h"
#include "stb_image.h"

#include "os/file.h"
#include "os/time.h"

#include "audio/audio_registry.h"
#include "audio/wav.h"

#include "render/render_registry.h"

struct Asset_Source_Callback_Data {
    Asset_Type asset_type = ASSET_NONE;
    const char *filter_ext = null;
};

static void init_asset_source_callback(const File_Callback_Data *data) {
    auto *ascd = (Asset_Source_Callback_Data *)data->user_data;
    const char *ext = str_char_from_end(data->path, '.');
    
    if (ascd->filter_ext && str_cmp(ascd->filter_ext, ext) == false) {      
        //log("File path from callback %s is not accepted due to extension filter fail %s", data->path, ascd->filter_ext);
        return;
    }

    if (ascd->asset_type == ASSET_SHADER_INCLUDE) {
        asset_shader_include_count += 1;
    }
        
    Asset_Source source;
    source.type = ascd->asset_type;
    source.last_write_time = data->last_write_time;
    
    str_copy(source.path, data->path);
    fix_directory_delimiters(source.path);
    
    char relative_path[MAX_PATH_SIZE];
    convert_to_relative_asset_path(relative_path, data->path);
    
    asset_source_table.add(cache_sid(relative_path), source);
}

static inline void init_asset_sources(const char *path, Asset_Type type, const char *filter_ext = null) {
    Asset_Source_Callback_Data ascd;
    ascd.asset_type = type;
    ascd.filter_ext = filter_ext;
    
    for_each_file(path, init_asset_source_callback, &ascd);
}

static u64 asset_source_table_hash(const sid &a) {
    return a;
}

void init_asset_source_table(Asset_Source_Table *table) {
    START_SCOPE_TIMER(init);

    *table = Asset_Source_Table(MAX_ASSETS);
    table->hash_function = &asset_source_table_hash;

    // @Fix: we can't control order of values in hash table like this.
    init_asset_sources(DIR_SHADERS,  ASSET_SHADER_INCLUDE, ".h");
    init_asset_sources(DIR_SHADERS,  ASSET_SHADER, get_desired_shader_file_extension());
    init_asset_sources(DIR_TEXTURES, ASSET_TEXTURE);
    init_asset_sources(DIR_SOUNDS,   ASSET_SOUND);
    init_asset_sources(DIR_FONTS,    ASSET_FONT);

    log("Initialized asset source table in %.2fms", CHECK_SCOPE_TIMER_MS(init));
}

static u64 asset_table_hash(const sid &a) {
    return a;
}

void init_asset_table(Asset_Table *table) {
    constexpr f32 scale = 2.0f - MAX_HASH_TABLE_LOAD_FACTOR;
    *table = Asset_Table((s32)(MAX_ASSETS * scale));
    table->hash_function = &asset_table_hash;
}

Texture_Memory load_texture_memory(const char *path, bool log_error) {
    Texture_Memory memory = {};
    
	u64 buffer_size = 0;
	void *buffer = allocl(MAX_TEXTURE_SIZE);
	defer { freel(MAX_TEXTURE_SIZE); };

	if (!read_file(path, buffer, MAX_TEXTURE_SIZE, &buffer_size, log_error)) {
		return {};
	}

	memory.data = stbi_load_from_memory((u8 *)buffer, (s32)buffer_size,
                                        &memory.width, &memory.height,
                                        &memory.channel_count, 0);
    
	if (!memory.data) {
		if (log_error) error("Failed to load texture %s, stbi reason %s", path, stbi_failure_reason());
		return {};
	}

    Assert(memory.width * memory.height * memory.channel_count <= MAX_TEXTURE_SIZE);
    
    return memory;
}

void free_texture_memory(Texture_Memory *memory) {
    stbi_image_free(memory->data);
    *memory = {};
}

Sound_Memory load_sound_memory(const char *path) {
    Sound_Memory memory = {};

    u64 buffer_size = 0;
	void *buffer = allocl(MAX_SOUND_SIZE);
    if (!read_file(path, buffer, MAX_SOUND_SIZE, &buffer_size)) {
        freel(MAX_SOUND_SIZE);
        return {};
    }
    
	memory.data = extract_wav(buffer, &memory.channel_count, &memory.sample_rate, &memory.bit_depth, &memory.size);
	if (!memory.data) {
        freel(MAX_SOUND_SIZE);
        return {};
    }

    return memory;
}

void free_sound_memory(Sound_Memory *memory) {
    freel(MAX_SOUND_SIZE);
    *memory = {};
}

void save_asset_pack(const char *path) {
    START_SCOPE_TIMER(save);

    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_WRITE);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        log("Asset pack %s does not exist, creating new one", path);
        file = open_file(path, FILE_OPEN_NEW, FILE_FLAG_WRITE);
        if (file == INVALID_FILE) {
            error("Failed to create new asset pack %s", path);
            return;
        }
    }

    Asset_Pack_Header header;
    header.magic_value = ASSET_PACK_MAGIC_VALUE;
    header.version     = ASSET_PACK_VERSION;
    header.asset_count = asset_source_table.count;
    write_file(file, &header, sizeof(Asset_Pack_Header));

    const u64 asset_data_offset  = sizeof(Asset_Pack_Header);
    const u64 asset_data_size    = asset_source_table.count * sizeof(Asset);
    const u64 binary_data_offset = asset_data_offset + asset_data_size;
    
    set_file_pointer_position(file, binary_data_offset);

    auto *assets = (Asset *)allocl(asset_data_size);
    defer { freel(asset_source_table.count * sizeof(Asset)); };
    
    s32 count = 0;

    // @Cleanup: remove this loop, figure out how to load shader includes before shaders.
    For (asset_source_table) {
        if (it.value.type == ASSET_SHADER_INCLUDE) {
            auto &asset = assets[count];
            asset.type = it.value.type;
            asset.data_offset = get_file_pointer_position(file);
            asset.registry_index = INVALID_INDEX;
            convert_to_relative_asset_path(asset.relative_path, it.value.path);
            
            count += 1;
        
            void *buffer = allocl(MAX_SHADER_SIZE);
            u64 bytes_read = 0;
            read_file(it.value.path, buffer, MAX_SHADER_SIZE, &bytes_read);

            write_file(file, buffer, bytes_read);
            
            asset.as_shader_include.size = (u32)bytes_read;
            
            freel(MAX_SHADER_SIZE);
        }
    }
    
    For (asset_source_table) {
        auto &asset = assets[count];
        asset.type = it.value.type;
        asset.data_offset = get_file_pointer_position(file);
        asset.registry_index = INVALID_INDEX;
        convert_to_relative_asset_path(asset.relative_path, it.value.path);

        if (it.value.type != ASSET_SHADER_INCLUDE) // @Cleanup: temp if
            count += 1;
        
        switch (it.value.type) {
        case ASSET_SHADER: {           
            void *buffer = allocl(MAX_SHADER_SIZE);
            u64 bytes_read = 0;
            read_file(it.value.path, buffer, MAX_SHADER_SIZE, &bytes_read);

            write_file(file, buffer, bytes_read);
            
            asset.as_shader.size = (u32)bytes_read;
            
            freel(MAX_SHADER_SIZE);
            
            break;
        }
        case ASSET_SHADER_INCLUDE: {
            /*
            void *buffer = allocl(MAX_SHADER_SIZE);
            u64 bytes_read = 0;
            read_file(it.value.path, buffer, MAX_SHADER_SIZE, &bytes_read);

            write_file(file, buffer, bytes_read);
            
            asset.as_shader_include.size = (u32)bytes_read;
            
            freel(MAX_SHADER_SIZE);
            */
            break;
        }
        case ASSET_TEXTURE: {            
            Texture_Memory memory = load_texture_memory(it.value.path);
            if (!memory.data) {
                error("Failed to load texture memory from %s", it.value.path);
                break;
            }

            const u64 size = memory.width * memory.height * memory.channel_count;
            write_file(file, memory.data, size);

            asset.as_texture.width  = memory.width;
            asset.as_texture.height = memory.height;
            asset.as_texture.channel_count = memory.channel_count;
            
            free_texture_memory(&memory);
            break;
        }
        case ASSET_SOUND: {
            void *buffer = allocl(MAX_SOUND_SIZE);
            defer { freel(MAX_SOUND_SIZE); };
            
            if (read_file(it.value.path, buffer, MAX_SOUND_SIZE)) {
                Wav_Header wavh;
                void *sampled_data = parse_wav(buffer, &wavh);
                if (sampled_data) {
                    write_file(file, sampled_data, wavh.sampled_data_size);

                    asset.as_sound.size          = wavh.sampled_data_size;
                    asset.as_sound.sample_rate   = wavh.samples_per_second;
                    asset.as_sound.channel_count = wavh.channel_count;
                    asset.as_sound.bit_depth     = wavh.bits_per_sample;
                } else {
                    error("Failed to parse sound %s", it.value.path);
                }
            }

            /*
            Sound_Memory memory = load_sound_memory(it.value.path);
            if (!memory.data) {
                error("Failed to load sound memory from %s", it.value.path);
                break;
            }

            write_file(file, memory.data, memory.size);

            asset.as_sound.size          = memory.size;
            asset.as_sound.sample_rate   = memory.sample_rate;
            asset.as_sound.channel_count = memory.channel_count;
            asset.as_sound.bit_depth     = memory.bit_depth;
            
            free_sound_memory(&memory);
            */
            
            break;
        }
        case ASSET_FONT: {
            char *buffer = allocltn(char, MAX_FONT_SIZE);
            u64 bytes_read = 0;
            read_file(it.value.path, buffer, MAX_FONT_SIZE, &bytes_read);
            
            write_file(file, buffer, bytes_read);
            
            asset.as_font.size = (u32)bytes_read;
            
            freel(MAX_FONT_SIZE);
            
            break;
        }
        default: {
            error("Asset source for %s has invalid type", it.value.path);
            break;
        }
        }
    }

    set_file_pointer_position(file, asset_data_offset);
    write_file(file, assets, asset_data_size);

    log("Saved asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(save));
}

void load_asset_pack(const char *path, Asset_Table *table) {
    START_SCOPE_TIMER(load);

    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    Asset_Pack_Header header;
    read_file(file, &header, sizeof(Asset_Pack_Header));

    if (header.magic_value != ASSET_PACK_MAGIC_VALUE) {
        error("Wrong asset pack %s magic value %d", path, header.magic_value);
        return;
    }

    if (header.version != ASSET_PACK_VERSION) {
        error("Wrong asset pack %s version %d", path, header.version);
        return;
    }
    
    const u64 asset_data_offset = sizeof(Asset_Pack_Header);
    set_file_pointer_position(file, asset_data_offset);

    // This buffer is temp allocated for all shader include headers that will
    // be used/reused during shader creation and freed at the end of asset pack load,
    // which obviously indicated that we are done loading game assets.
    //
    // So, this is very important that all shader includes should come before shader
    // sources (or at least those who use them) in asset pack file.
    const s32 MAX_SHADER_INCLUDE_BUFFER_SIZE = asset_shader_include_count * MAX_SHADER_SIZE;
    s32 shader_include_buffer_size = 0;
    void *shader_include_buffer = allocl(MAX_SHADER_INCLUDE_BUFFER_SIZE);

#if DEVELOPER
    // Clear buffer only in game build without hot reload feature.
    defer { freel(MAX_SHADER_INCLUDE_BUFFER_SIZE); };
#endif
    
    for (u32 i = 0; i < header.asset_count; ++i) {
        Asset asset;
        read_file(file, &asset, sizeof(Asset));

        switch (asset.type) {
        case ASSET_SHADER: {
            const auto &shader = asset.as_shader;

            void *data = allocl(shader.size);
            defer { freel(shader.size); };

            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, shader.size);
            set_file_pointer_position(file, last_position);
            
            asset.registry_index = create_shader((char *)data, asset.relative_path);
            
            break;
        }
        case ASSET_SHADER_INCLUDE: {
            auto &shader_include = asset.as_shader_include;

            void *data = alloclp(&shader_include_buffer, MAX_SHADER_SIZE);
            shader_include_buffer_size += MAX_SHADER_SIZE;
            Assert(shader_include_buffer_size <= MAX_SHADER_INCLUDE_BUFFER_SIZE);

            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, shader_include.size);
            set_file_pointer_position(file, last_position);

            shader_include.source = (char *)data;
            
            break;
        }
        case ASSET_TEXTURE: {
            const auto &texture = asset.as_texture;
            const u64 size = texture.width * texture.height * texture.channel_count;

            void *data = allocl(size);
            defer { freel(size); };
            
            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, size);
            set_file_pointer_position(file, last_position);

            const auto format = get_desired_texture_format(texture.channel_count);            
            asset.registry_index = create_texture(TEXTURE_TYPE_2D, format, texture.width, texture.height, data);

            // @Cleanup: hardcoded!
            generate_texture_mipmaps(asset.registry_index);
            set_texture_wrap(asset.registry_index, TEXTURE_WRAP_REPEAT);
            set_texture_filter(asset.registry_index, TEXTURE_FILTER_NEAREST);
    
            break;
        }
        case ASSET_SOUND: {
            const auto &sound = asset.as_sound;

            void *data = allocl(sound.size);
            defer { freel(sound.size); };
            
            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, sound.size);
            set_file_pointer_position(file, last_position);

            asset.registry_index = create_sound(sound.sample_rate, sound.channel_count, sound.bit_depth, data, sound.size, 0);
            
            break;
        }
        case ASSET_FONT: {
            auto &font = asset.as_font;

            void *data = allocl(MAX_FONT_SIZE);

            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, font.size);
            set_file_pointer_position(file, last_position);

            font.data = (char *)data;
            
            break;
        }
        default: {
            error("Invalid asset type %d in asset %s", asset.type, asset.relative_path);
            break;
        }
        }

        asset_table.add(cache_sid(asset.relative_path), asset);
    }

    log("Loaded asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void convert_to_relative_asset_path(char *relative_path, const char *full_path) {
    const char *data = str_sub(full_path, "\\data");
    if (!data) data = str_sub(full_path, "/data");
    if (!data) return;

    str_copy(relative_path, data);
    fix_directory_delimiters(relative_path);
}

void convert_to_full_asset_path(char *full_path, const char *relative_path) {
    str_copy(full_path, DIR_RUN_TREE);
    str_glue(full_path, relative_path);
    fix_directory_delimiters(full_path);
}
