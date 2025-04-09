#include "pch.h"
#include "asset_registry.h"
#include "log.h"
#include "sid.h"
#include "assertion.h"
#include "profile.h"
#include "stb_image.h"

#include "os/file.h"
#include "os/time.h"

#include "audio/audio_registry.h"
#include "render/render_registry.h"

static u64 asset_table_hash(const sid &a) {
    return a;
}

static void init_asset_source_callback(const File_Callback_Data *callback_data) {
    Asset_Source source = {};
    source.type = *(Asset_Type *)callback_data->user_data;

    assert(strlen(callback_data->path) <= MAX_PATH_SIZE);
    strcpy(source.path, callback_data->path);

    asset_sources.add(source);
}

static inline void init_assets_from_folder(const char *path, Asset_Type type) {
    for_each_file(path, init_asset_source_callback, &type);
}

void init_asset_sources(Asset_Source_List *sources) {
    START_SCOPE_TIMER(init);

    *sources = Asset_Source_List(MAX_ASSETS);

    init_assets_from_folder(SHADER_FOLDER,  ASSET_SHADER);
    init_assets_from_folder(TEXTURE_FOLDER, ASSET_TEXTURE);
    init_assets_from_folder(SOUND_FOLDER,   ASSET_SOUND);

    log("Initialized asset sources in %.2fms", CHECK_SCOPE_TIMER_MS(init));
}

void init_asset_table(Asset_Table *table) {
    constexpr f32 scale = 2.0f - ACCEPTABLE_HASH_TABLE_LOAD_FACTOR;
    *table = Asset_Table((s32)(MAX_ASSETS * scale));
    table->hash_function = &asset_table_hash;
}

Texture_Memory load_texture_memory(const char *path) {
    Texture_Memory memory = {};
    
	u64 buffer_size = 0;
	void *buffer = push(temp, MAX_TEXTURE_SIZE);
	defer { pop(temp, MAX_TEXTURE_SIZE); };

	if (!read_file(path, buffer, MAX_TEXTURE_SIZE, &buffer_size)) {
		return {};
	}

	memory.data = stbi_load_from_memory((u8 *)buffer, (s32)buffer_size,
                                        &memory.width, &memory.height,
                                        &memory.channel_count, 0);
    
	if (!memory.data) {
		error("Failed to load texture %s, stbi reason %s", path, stbi_failure_reason());
		return {};
	}

    assert(memory.width * memory.height * memory.channel_count <= MAX_TEXTURE_SIZE);
    
    return memory;
}

void free_texture_memory(Texture_Memory *memory) {
    stbi_image_free(memory->data);
    *memory = {};
}

Sound_Memory load_sound_memory(const char *path) {
    Sound_Memory memory = {};

    u64 buffer_size = 0;
	void *buffer = push(temp, MAX_SOUND_SIZE);
    if (!read_file(path, buffer, MAX_SOUND_SIZE, &buffer_size)) {
        pop(temp, MAX_SOUND_SIZE);
        return {};
    }
    
	memory.data = extract_wav(buffer, &memory.channel_count, &memory.sample_rate, &memory.bit_depth, &memory.size);
	if (!memory.data) {
        pop(temp, MAX_SOUND_SIZE);
        return {};
    }

    return memory;
}

void free_sound_memory(Sound_Memory *memory) {
    pop(temp, MAX_SOUND_SIZE);
    *memory = {};
}

void save_asset_pack(const char *path) {
    START_SCOPE_TIMER(load);

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

    Asset_Pack_Header header = {};
    header.magic_value = ASSET_PACK_MAGIC_VALUE;
    header.version     = ASSET_PACK_VERSION;
    header.asset_count = asset_sources.count;
    write_file(file, &header, sizeof(Asset_Pack_Header));

    const u64 asset_data_offset  = sizeof(Asset_Pack_Header);
    const u64 asset_data_size    = asset_sources.count * sizeof(Asset);
    const u64 binary_data_offset = asset_data_offset + asset_data_size;
    
    set_file_pointer_position(file, binary_data_offset);

    auto *assets = (Asset *)push(temp, asset_data_size);
    defer { pop_array(temp, asset_sources.count, Asset); };
    
    for (s32 i = 0; i < asset_sources.count; ++i) {
        const auto *source = asset_sources.items + i;

        auto *asset = assets + i;
        asset->type = source->type;
        asset->data_offset = get_file_pointer_position(file);
        asset->registry_index = INVALID_INDEX;
        
        strcpy(asset->relative_path, source->path);
        convert_to_relative_asset_path(asset->relative_path);
        
        switch (source->type) {
        case ASSET_SHADER: {
            void *buffer = push(temp, MAX_SHADER_SIZE);
            u64 bytes_read = 0;
            read_file(source->path, buffer, MAX_SHADER_SIZE, &bytes_read);

            write_file(file, buffer, bytes_read);
            
            asset->as_shader.size = (u32)bytes_read;
            
            pop(temp, MAX_SHADER_SIZE);
            break;
        }
        case ASSET_TEXTURE: {            
            Texture_Memory memory = load_texture_memory(source->path);
            if (!memory.data) {
                error("Failed to load texture memory from %s", source->path);
                break;
            }

            const u64 size = memory.width * memory.height * memory.channel_count;
            write_file(file, memory.data, size);

            asset->as_texture.width  = memory.width;
            asset->as_texture.height = memory.height;
            asset->as_texture.channel_count = memory.channel_count;
            
            free_texture_memory(&memory);
            break;
        }
        case ASSET_SOUND: {
            Sound_Memory memory = load_sound_memory(source->path);
            if (!memory.data) {
                error("Failed to load sound memory from %s", source->path);
                break;
            }

            write_file(file, memory.data, memory.size);

            asset->as_sound.size          = memory.size;
            asset->as_sound.sample_rate   = memory.sample_rate;
            asset->as_sound.channel_count = memory.channel_count;
            asset->as_sound.bit_depth     = memory.bit_depth;
            
            free_sound_memory(&memory);
            break;
        }
        default: {
            error("Asset source for %s has no valid type", source->path);
            break;
        }
        }
    }

    set_file_pointer_position(file, asset_data_offset);
    write_file(file, assets, asset_data_size);

    log("Saved asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void load_asset_pack(const char *path, Asset_Table *table) {
    START_SCOPE_TIMER(load);
    const s64 counter = performance_counter();

    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    Asset_Pack_Header header = {};
    read_file(file, &header, sizeof(Asset_Pack_Header));

    if (header.magic_value != ASSET_PACK_MAGIC_VALUE) {
        error("Wrong asset pack magic value %d", header.magic_value);
        return;
    }

    if (header.version != ASSET_PACK_VERSION) {
        error("Wrong asset pack version %d", header.version);
        return;
    }
    
    const u64 asset_data_offset = sizeof(Asset_Pack_Header);
    set_file_pointer_position(file, asset_data_offset);

    for (u32 i = 0; i < header.asset_count; ++i) {
        Asset asset;
        read_file(file, &asset, sizeof(Asset));

        switch (asset.type) {
        case ASSET_SHADER: {
            const auto &shader = asset.as_shader;

            void *data = push(temp, shader.size);
            defer { pop(temp, shader.size); };

            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, shader.size);
            set_file_pointer_position(file, last_position);

            asset.registry_index = create_shader((char *)data);
            
            break;
        }
        case ASSET_TEXTURE: {
            const auto &texture = asset.as_texture;
            const u64 size = texture.width * texture.height * texture.channel_count;

            void *data = push(temp, size);
            defer { pop(temp, size); };
            
            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, size);
            set_file_pointer_position(file, last_position);

            Texture_Format_Type format;
            if (texture.channel_count == 3)      format = TEXTURE_FORMAT_RGB_8;
            else if (texture.channel_count == 4) format = TEXTURE_FORMAT_RGBA_8;
            else assert(false); // @Temp
            
            asset.registry_index = create_texture(TEXTURE_TYPE_2D, format, texture.width, texture.height, data);

            // @Cleanup: hardcoded!
            generate_texture_mipmaps(asset.registry_index);
            set_texture_wrap(asset.registry_index, TEXTURE_WRAP_REPEAT);
            set_texture_filter(asset.registry_index, TEXTURE_FILTER_NEAREST);
    
            break;
        }
        case ASSET_SOUND: {
            const auto &sound = asset.as_sound;

            void *data = push(temp, sound.size);
            defer { pop(temp, sound.size); };
            
            const u64 last_position = get_file_pointer_position(file);
            set_file_pointer_position(file, asset.data_offset);
            read_file(file, data, sound.size);
            set_file_pointer_position(file, last_position);

            asset.registry_index = create_sound(sound.sample_rate, sound.channel_count, sound.bit_depth, data, sound.size, 0);
            
            break;
        }
        default: {
            error("Invalid asset type %d in asset description %d", asset.type, i);
            break;
        }
        }

        asset_table.add(cache_sid(asset.relative_path), asset);
    }

    log("Loaded asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void convert_to_relative_asset_path(char *path) {
    const char *data = strstr(path, "\\data");
    if (!data) data = strstr(path, "/data");
    if (!data) return;

    strcpy(path, data);
    fix_directory_delimiters(path);
}

void convert_to_full_asset_path(const char *relative_path, char *full_path) {
    strcpy(full_path, RUN_TREE_FOLDER);
    strcat(full_path, relative_path);
    fix_directory_delimiters(full_path);
}

void save_level(const char *path, World *world) {
    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_WRITE);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        log("Level file %s does not exist, creating new one", path);
        file = open_file(path, FILE_OPEN_NEW, FILE_FLAG_WRITE);
        if (file == INVALID_FILE) {
            error("Failed to create new level file %s", path);
            return;
        }
    }

    
}

void load_level(const char *path, World *world) {
    
}
