#include "pch.h"
#include "asset.h"
#include "log.h"
#include "sid.h"
#include "str.h"
#include "profile.h"
#include "font.h"
#include "flip_book.h"

#include "stb_image.h"
#include "tiny_obj_loader.h"

#include "os/file.h"
#include "os/time.h"

#include "audio/wav.h"
#include "audio/sound.h"

#include "render/buffer_storage.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/material.h"
#include "render/uniform.h"
#include "render/mesh.h"

#include "math/matrix.h"

const char *TYPE_NAME_U32  = "u32";
const char *TYPE_NAME_F32  = "f32";
const char *TYPE_NAME_VEC2 = "vec2";
const char *TYPE_NAME_VEC3 = "vec3";
const char *TYPE_NAME_VEC4 = "vec4";
const char *TYPE_NAME_MAT4 = "mat4";

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

    char full_path[MAX_PATH_SIZE];
    str_copy(full_path, data->path);
    fix_directory_delimiters(full_path);

    char relative_path[MAX_PATH_SIZE];
    convert_to_relative_asset_path(relative_path, full_path);

    const sid sid_relative_path = sid_intern(relative_path);
    
    Asset_Source source;
    source.asset_type = ascd->asset_type;
    source.last_write_time = data->last_write_time;
    source.sid_full_path = sid_intern(full_path);
    source.sid_relative_path = sid_relative_path;
    
    auto &ast = asset_source_table;
    ast.table.add(sid_relative_path, source);
    ast.count_by_type[source.asset_type] += 1;
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

void init_asset_source_table() {
    START_SCOPE_TIMER(init);

    auto &ast = asset_source_table;
    
    ast.table = Hash_Table<sid, Asset_Source>(MAX_ASSET_SOURCES);
    ast.table.hash_function = &asset_source_table_hash;
    set_bytes(ast.count_by_type, 0, sizeof(ast.count_by_type));
    
    // @Fix: we can't control order of values in hash table like this.
    init_asset_sources(DIR_SHADERS,    ASSET_SHADER_INCLUDE, ".h");
    init_asset_sources(DIR_SHADERS,    ASSET_SHADER, get_desired_shader_file_extension());
    init_asset_sources(DIR_TEXTURES,   ASSET_TEXTURE);
    init_asset_sources(DIR_MATERIALS,  ASSET_MATERIAL);
    init_asset_sources(DIR_MESHES,     ASSET_MESH);
    init_asset_sources(DIR_SOUNDS,     ASSET_SOUND);
    init_asset_sources(DIR_FONTS,      ASSET_FONT);
    init_asset_sources(DIR_FLIP_BOOKS, ASSET_FLIP_BOOK);

    log("Initialized asset source table in %.2fms", CHECK_SCOPE_TIMER_MS(init));
}

static u64 asset_table_hash(const sid &a) {
    return a;
}

void init_asset_table() {
    constexpr f32 scale = 2.0f - MAX_HASH_TABLE_LOAD_FACTOR;

    auto &table = asset_table;

    table.shader_includes = Hash_Table<sid, Shader_Include>((s32)(MAX_SHADER_INCLUDES * scale));
    table.shader_includes.hash_function = &asset_table_hash;
    
    table.shaders = Hash_Table<sid, Shader>((s32)(MAX_SHADERS * scale));
    table.shaders.hash_function = &asset_table_hash;

    table.textures = Hash_Table<sid, Texture>((s32)(MAX_TEXTURES * scale));
    table.textures.hash_function = &asset_table_hash;

    table.materials = Hash_Table<sid, Material>((s32)(MAX_MATERIALS * scale));
    table.materials.hash_function = &asset_table_hash;

    table.meshes = Hash_Table<sid, Mesh>((s32)(MAX_MESHES * scale));
    table.meshes.hash_function = &asset_table_hash;

    table.sounds = Hash_Table<sid, Sound>((s32)(MAX_SOUNDS * scale));
    table.sounds.hash_function = &asset_table_hash;

    table.fonts = Hash_Table<sid, Font>((s32)(MAX_FONTS * scale));
    table.fonts.hash_function = &asset_table_hash;

    table.flip_books = Hash_Table<sid, Flip_Book>((s32)(MAX_FLIP_BOOKS * scale));
    table.flip_books.hash_function = &asset_table_hash;
}

void save_asset_pack(const char *path) {
    START_SCOPE_TIMER(save);

    File file = os_file_open(path, FILE_OPEN_EXISTING, FILE_FLAG_WRITE);
    defer { os_file_close(file); };
    
    if (file == INVALID_FILE) {
        log("Asset pack %s does not exist, creating new one", path);
        file = os_file_open(path, FILE_OPEN_NEW, FILE_FLAG_WRITE);
        if (file == INVALID_FILE) {
            error("Failed to create new asset pack %s", path);
            return;
        }
    }
    
    auto &ast = asset_source_table;

    Asset_Pack_Header header;
    header.magic_value = ASSET_PACK_MAGIC_VALUE;
    header.version     = ASSET_PACK_VERSION;
    copy_bytes(header.count_by_type, ast.count_by_type, sizeof(header.count_by_type));

    u64 offset_by_type[ASSET_TYPE_COUNT];
    u64 offset = sizeof(Asset_Pack_Header);
    for (u8 i = 0; i < ASSET_TYPE_COUNT; ++i) {
        offset_by_type[i] = offset;
        offset += get_asset_type_size((Asset_Type)i) * ast.count_by_type[i];
    }

    copy_bytes(header.offset_by_type, offset_by_type, sizeof(header.offset_by_type));

    header.data_offset = offset;
    
    os_file_write(file, &header, sizeof(Asset_Pack_Header));

    For (ast.table) {
        const char *path      = sid_str(it.value.sid_relative_path);
        const char *full_path = sid_str(it.value.sid_full_path);
        
        const auto asset_type = it.value.asset_type;
        Assert(asset_type >= 0 && asset_type < ASSET_TYPE_COUNT);

        const u32 asset_size = get_asset_type_size(asset_type);
        const u64 asset_offset = offset_by_type[asset_type];
        offset_by_type[asset_type] += asset_size;
        
        os_file_set_pointer_position(file, asset_offset);
        
        switch (asset_type) {
        case ASSET_SHADER_INCLUDE: {
            Shader_Include include = {};

            void *buffer = allocl(MAX_SHADER_SIZE);
            defer { freel(MAX_SHADER_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_SHADER_SIZE, &bytes_read);

            include.data_offset = offset;
            include.data_size = bytes_read;
            offset += include.data_size;

            include.path_offset = offset;
            include.path_size = str_size(path) + 1;
            offset += include.path_size;

            os_file_write(file, &include, asset_size);

            os_file_set_pointer_position(file, include.data_offset);
            os_file_write(file, buffer, include.data_size);

            os_file_set_pointer_position(file, include.path_offset);
            os_file_write(file, path, include.path_size);

            break;
        }
        case ASSET_SHADER: {
            Shader shader = {};

            void *buffer = allocl(MAX_SHADER_SIZE);
            defer { freel(MAX_SHADER_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_SHADER_SIZE, &bytes_read);

            shader.data_offset = offset;
            shader.data_size = bytes_read;
            offset += shader.data_size;

            shader.path_offset = offset;
            shader.path_size = str_size(path) + 1;
            offset += shader.path_size;

            os_file_write(file, &shader, asset_size);

            os_file_set_pointer_position(file, shader.data_offset);
            os_file_write(file, buffer, shader.data_size);

            os_file_set_pointer_position(file, shader.path_offset);
            os_file_write(file, path, shader.path_size);
            
            break;
        }
        case ASSET_TEXTURE: {
            Texture texture = {};

            void *buffer = allocl(MAX_TEXTURE_SIZE);
            defer { freel(MAX_TEXTURE_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_TEXTURE_SIZE, &bytes_read);

            void *data = stbi_load_from_memory((u8 *)buffer, (s32)bytes_read, &texture.width, &texture.height, &texture.channel_count, 0);
            defer { stbi_image_free(data); };

            texture.type = TEXTURE_TYPE_2D; // @Cleanup: detect texture type.
            texture.format = get_texture_format_from_channel_count(texture.channel_count);

            texture.data_offset = offset;
            texture.data_size = texture.width * texture.height * texture.channel_count;
            offset += texture.data_size;

            texture.path_offset = offset;
            texture.path_size = str_size(path) + 1;
            offset += texture.path_size;

            os_file_write(file, &texture, asset_size);

            os_file_set_pointer_position(file, texture.data_offset);
            os_file_write(file, data, texture.data_size);
            
            os_file_set_pointer_position(file, texture.path_offset);
            os_file_write(file, path, texture.path_size);
            
            break;
        }
        case ASSET_MATERIAL: {
            Material material = {};

            void *buffer = allocl(MAX_MATERIAL_SIZE);
            defer { freel(MAX_MATERIAL_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_MATERIAL_SIZE, &bytes_read);

            material.data_offset = offset;
            material.data_size = bytes_read;
            offset += material.data_size;

            material.path_offset = offset;
            material.path_size = str_size(path) + 1;
            offset += material.path_size;

            os_file_write(file, &material, asset_size);

            os_file_set_pointer_position(file, material.data_offset);
            os_file_write(file, buffer, material.data_size);

            os_file_set_pointer_position(file, material.path_offset);
            os_file_write(file, path, material.path_size);
            
            break;
        }
        case ASSET_MESH: {
            Mesh mesh = {};

            void *buffer = allocl(MAX_MESH_SIZE);
            defer { freel(MAX_MESH_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_MESH_SIZE, &bytes_read);

            mesh.data_offset = offset;
            mesh.data_size = bytes_read;
            offset += mesh.data_size;

            mesh.path_offset = offset;
            mesh.path_size = str_size(path) + 1;
            offset += mesh.path_size;
            
            os_file_write(file, &mesh, asset_size);

            os_file_set_pointer_position(file, mesh.data_offset);
            os_file_write(file, buffer, mesh.data_size);
            
            os_file_set_pointer_position(file, mesh.path_offset);
            os_file_write(file, path, mesh.path_size);
            
            break;
        }
        case ASSET_SOUND: {
            Sound sound = {};

            void *buffer = allocl(MAX_SOUND_SIZE);
            defer { freel(MAX_SOUND_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_SOUND_SIZE, &bytes_read);

            Wav_Header wav;
            void *sampled_data = parse_wav(buffer, &wav);

            sound.channel_count = wav.channel_count;
            sound.sample_rate = wav.samples_per_second;
            sound.bit_rate = wav.bits_per_sample;

            sound.data_offset = offset;
            sound.data_size = wav.sampled_data_size;
            offset += sound.data_size;

            sound.path_offset = offset;
            sound.path_size = str_size(path) + 1;
            offset += sound.path_size;
            
            os_file_write(file, &sound, asset_size);

            os_file_set_pointer_position(file, sound.data_offset);
            os_file_write(file, buffer, sound.data_size);
                        
            os_file_set_pointer_position(file, sound.path_offset);
            os_file_write(file, path, sound.path_size);
                        
            break;
        }
        case ASSET_FONT: {
            Font font = {};

            void *buffer = allocl(MAX_FONT_SIZE);
            defer { freel(MAX_FONT_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_FONT_SIZE, &bytes_read);

            font.data_offset = offset;
            font.data_size = bytes_read;
            offset += font.data_size;

            font.path_offset = offset;
            font.path_size = str_size(path) + 1;
            offset += font.path_size;
                        
            os_file_write(file, &font, asset_size);

            os_file_set_pointer_position(file, font.data_offset);
            os_file_write(file, buffer, font.data_size);

            os_file_set_pointer_position(file, font.path_offset);
            os_file_write(file, path, font.path_size);
            
            break;
        }
        case ASSET_FLIP_BOOK: {
            Flip_Book flip_book = {};

            void *buffer = allocl(MAX_FLIP_BOOK_SIZE);
            defer { freel(MAX_FLIP_BOOK_SIZE); };

            u64 bytes_read = 0;
            os_file_read(full_path, buffer, MAX_FONT_SIZE, &bytes_read);

            flip_book.data_offset = offset;
            flip_book.data_size = bytes_read;
            offset += flip_book.data_size;

            flip_book.path_offset = offset;
            flip_book.path_size = str_size(path) + 1;
            offset += flip_book.path_size;
            
            os_file_write(file, &flip_book, asset_size);

            os_file_set_pointer_position(file, flip_book.data_offset);
            os_file_write(file, buffer, flip_book.data_size);
            
            os_file_set_pointer_position(file, flip_book.path_offset);
            os_file_write(file, path, flip_book.path_size);
            
            break;
        }
        }
    }

    log("Saved asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(save));
}

void load_asset_pack(const char *path) {
    START_SCOPE_TIMER(load);

    File file = os_file_open(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
    defer { os_file_close(file); };
    
    if (file == INVALID_FILE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    Asset_Pack_Header header;
    os_file_read(file, &header, sizeof(Asset_Pack_Header));

    if (header.magic_value != ASSET_PACK_MAGIC_VALUE) {
        error("Wrong asset pack %s magic value %d", path, header.magic_value);
        return;
    }

    if (header.version != ASSET_PACK_VERSION) {
        error("Wrong asset pack %s version %d", path, header.version);
        return;
    }
    
    // This buffer is temp allocated for all shader include headers that will
    // be used/reused during shader creation and freed at the end of asset pack load,
    // which obviously indicated that we are done loading game assets.
    //
    // So, this is very important that all shader includes should come before shader
    // sources (or at least those who use them) in asset pack file.
    const u64 MAX_SHADER_INCLUDE_BUFFER_SIZE = header.count_by_type[ASSET_SHADER_INCLUDE] * MAX_SHADER_SIZE;
    u64 shader_include_buffer_size = 0;
    void *shader_include_buffer = allocl(MAX_SHADER_INCLUDE_BUFFER_SIZE);

#if DEVELOPER
    // Clear buffer only in game build without hot reload feature.
    defer { freel(MAX_SHADER_INCLUDE_BUFFER_SIZE); };
#endif

    for (u8 i = 0; i < ASSET_TYPE_COUNT; ++i) {
        char path[MAX_PATH_SIZE];
            
        const auto asset_type  = (Asset_Type)i;
        const s32 asset_count  = header.count_by_type[i];
        const u64 asset_offset = header.offset_by_type[i];
        const u32 asset_size   = get_asset_type_size(asset_type);

        for (s32 j = 0; j < asset_count; ++j) {
            os_file_set_pointer_position(file, asset_offset + asset_size * j);

            switch (asset_type) {
            case ASSET_SHADER_INCLUDE: {
                Shader_Include include = {};
                os_file_read(file, &include, asset_size);

                os_file_set_pointer_position(file, include.path_offset);
                os_file_read(file, path, include.path_size);
                include.sid_path = sid_intern(path);
                
                void *data = alloclp(&shader_include_buffer, include.data_size);
                shader_include_buffer_size += include.data_size;
                Assert(shader_include_buffer_size <= MAX_SHADER_INCLUDE_BUFFER_SIZE);

                os_file_set_pointer_position(file, include.data_offset);
                os_file_read(file, data, include.data_size);

                include.data = data;

                asset_table.shader_includes.add(include.sid_path, include);
                
                break;
            }
            case ASSET_SHADER: {                
                Shader shader = {};
                os_file_read(file, &shader, asset_size);

                os_file_set_pointer_position(file, shader.path_offset);
                os_file_read(file, path, shader.path_size);
                shader.sid_path = sid_intern(path);
                
                void *data = allocl(shader.data_size);
                defer { freel(shader.data_size); };

                os_file_set_pointer_position(file, shader.data_offset);
                os_file_read(file, data, shader.data_size);

                init_shader_asset(&shader, data);

                asset_table.shaders.add(shader.sid_path, shader);
                
                break;
            }
            case ASSET_TEXTURE: {
                Texture texture = {};
                os_file_read(file, &texture, asset_size);

                os_file_set_pointer_position(file, texture.path_offset);
                os_file_read(file, path, texture.path_size);
                texture.sid_path = sid_intern(path);
                
                void *data = allocl(texture.data_size);
                defer { freel(texture.data_size); };
                
                os_file_set_pointer_position(file, texture.data_offset);
                os_file_read(file, data, texture.data_size);
                
                init_texture_asset(&texture, data);

                // @Cleanup: hardcoded!
                generate_texture_mipmaps(&texture);
                set_texture_wrap(&texture, TEXTURE_WRAP_REPEAT);
                set_texture_filter(&texture, TEXTURE_FILTER_NEAREST);

                asset_table.textures.add(texture.sid_path, texture);

                break;
            }
            case ASSET_MATERIAL: {
                Material material = {};
                os_file_read(file, &material, asset_size);

                os_file_set_pointer_position(file, material.path_offset);
                os_file_read(file, path, material.path_size);
                material.sid_path = sid_intern(path);
                
                void *data = allocl(material.data_size);
                defer { freel(material.data_size); };

                os_file_set_pointer_position(file, material.data_offset);
                os_file_read(file, data, material.data_size);

                init_material_asset(&material, data);
                
                asset_table.materials.add(material.sid_path, material);

                break;
            }
            case ASSET_MESH: {
                Mesh mesh = {};
                os_file_read(file, &mesh, asset_size);

                os_file_set_pointer_position(file, mesh.path_offset);
                os_file_read(file, path, mesh.path_size);
                mesh.sid_path = sid_intern(path);
                
                void *data = allocl(mesh.data_size);
                defer { freel(mesh.data_size); };

                os_file_set_pointer_position(file, mesh.data_offset);
                os_file_read(file, data, mesh.data_size);

                init_mesh_asset(&mesh, data);
                
                asset_table.meshes.add(mesh.sid_path, mesh);

                break;
            }
            case ASSET_SOUND: {
                Sound sound = {};
                os_file_read(file, &sound, asset_size);

                os_file_set_pointer_position(file, sound.path_offset);
                os_file_read(file, path, sound.path_size);
                sound.sid_path = sid_intern(path);
                
                void *data = allocl(sound.data_size);
                defer { freel(sound.data_size); };

                os_file_set_pointer_position(file, sound.data_offset);
                os_file_read(file, data, sound.data_size);

                init_sound_asset(&sound, data);
                
                asset_table.sounds.add(sound.sid_path, sound);

                break;
            }
            case ASSET_FONT: {
                Font font = {};
                os_file_read(file, &font, asset_size);

                os_file_set_pointer_position(file, font.path_offset);
                os_file_read(file, path, font.path_size);
                font.sid_path = sid_intern(path);
                
                void *data = allocl(font.data_size);

                os_file_set_pointer_position(file, font.data_offset);
                os_file_read(file, data, font.data_size);

                font.data = data;

                asset_table.fonts.add(font.sid_path, font);

                break;
            }
            case ASSET_FLIP_BOOK: {
                Flip_Book flip_book = {};
                os_file_read(file, &flip_book, asset_size);

                os_file_set_pointer_position(file, flip_book.path_offset);
                os_file_read(file, path, flip_book.path_size);
                flip_book.sid_path = sid_intern(path);
                
                void *data = allocl(flip_book.data_size);
                defer { freel(flip_book.data_size); };

                os_file_set_pointer_position(file, flip_book.data_offset);
                os_file_read(file, data, flip_book.data_size);

                init_flip_book_asset(&flip_book, data);
                
                asset_table.flip_books.add(flip_book.sid_path, flip_book);

                break;
            }
            }
        }
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

u32 get_asset_type_size(Asset_Type type) {
    switch (type) {
    case ASSET_SHADER_INCLUDE: return sizeof(Shader_Include);
    case ASSET_SHADER:         return sizeof(Shader);
    case ASSET_TEXTURE:        return sizeof(Texture);
    case ASSET_MATERIAL:       return sizeof(Material);
    case ASSET_MESH:           return sizeof(Mesh);
    case ASSET_SOUND:          return sizeof(Sound);
    case ASSET_FONT:           return sizeof(Font);
    case ASSET_FLIP_BOOK:      return sizeof(Flip_Book);
    default:
        error("Failed to get asset size from type %d", type);
        return 0;
    }
}


static void init_mesh_asset_mesh(Mesh *mesh, void *data) {
    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;
    
    enum Declaration_Type {
        DECL_NONE,
        DECL_VERTEX_COUNT,
        DECL_INDEX_COUNT,
        DECL_VERTEX_COMPONENT,
        DECL_DATA_GO,
        DECL_POSITION,
        DECL_NORMAL,
        DECL_INDEX,
        DECL_UV,
        
        DECL_COUNT
    };

    const char *DECL_NAME_VERTEX_COUNT      = "vn";
    const char *DECL_NAME_INDEX_COUNT       = "in";
    const char *DECL_NAME_VERTEX_COMPONENT  = "vc";
    const char *DECL_NAME_DATA_GO = "go";
    const char *DECL_NAME_POSITION = "p";
    const char *DECL_NAME_NORMAL   = "n";
    const char *DECL_NAME_UV       = "uv";
    const char *DECL_NAME_INDEX = "i";
    
    const char *DELIMITERS = " ";
    
    mesh->vertex_layout_size = 0;
    
    char *p = (char *)data;
    p[mesh->data_size] = '\0';

    u8 *r_vertex_data = (u8 *)vertex_buffer_storage.mapped_data;
    u8 *r_index_data  = (u8 *)index_buffer_storage.mapped_data;
    
    u32 vertex_offsets[MAX_MESH_VERTEX_COMPONENTS];

    s32 decl_to_vertex_layout_index[DECL_COUNT];
    Vertex_Component_Type decl_to_vct[DECL_COUNT];

    u32 index_data_offset = 0;

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_VERTEX_COUNT)) {
                decl_type = DECL_VERTEX_COUNT;
            } else if (str_cmp(token, DECL_NAME_INDEX_COUNT)) {
                decl_type = DECL_INDEX_COUNT;
            } else if (str_cmp(token, DECL_NAME_VERTEX_COMPONENT)) {
                decl_type = DECL_VERTEX_COMPONENT;
            } else if (str_cmp(token, DECL_NAME_DATA_GO)) {
                decl_type = DECL_DATA_GO;
            } else if (str_cmp(token, DECL_NAME_POSITION)) {
                decl_type = DECL_POSITION;
            } else if (str_cmp(token, DECL_NAME_NORMAL)) {
                decl_type = DECL_NORMAL;
            } else if (str_cmp(token, DECL_NAME_UV)) {
                decl_type = DECL_UV;
            } else if (str_cmp(token, DECL_NAME_INDEX)) {
                decl_type = DECL_INDEX;
            } else {
                error("Unknown mesh token declaration '%s'", token);
                continue;
            }
            
            switch (decl_type) {
            case DECL_VERTEX_COUNT: {
                char *sv = str_token(null, DELIMITERS);
                mesh->vertex_count = str_to_u32(sv);
                break;
            }
            case DECL_INDEX_COUNT: {
                char *sv = str_token(null, DELIMITERS);
                mesh->index_count = str_to_u32(sv);
                break;
            }
            case DECL_VERTEX_COMPONENT: {
                char *sdecl = str_token(null, DELIMITERS);
                char *stype = str_token(null, DELIMITERS);

                Vertex_Component_Type vct;
                if (str_cmp(stype, TYPE_NAME_VEC2)) {
                    vct = VERTEX_F32_2;
                } else if (str_cmp(stype, TYPE_NAME_VEC3)) {
                    vct = VERTEX_F32_3;                    
                } else if (str_cmp(stype, TYPE_NAME_VEC4)) {
                    vct = VERTEX_F32_4;
                } else {
                    error("Unknown mesh token vertex component type declaration '%s'", token);
                    continue;
                }

                Declaration_Type decl_vertex_type = DECL_NONE;
                if (str_cmp(sdecl, DECL_NAME_POSITION)) {
                    decl_vertex_type = DECL_POSITION;
                } else if (str_cmp(sdecl, DECL_NAME_NORMAL)) {
                    decl_vertex_type = DECL_NORMAL;
                } else if (str_cmp(sdecl, DECL_NAME_UV)) {
                    decl_vertex_type = DECL_UV;
                } else {
                    error("Unknown mesh token vertex component declaration '%s'", token);
                    continue;
                }

                decl_to_vct[decl_vertex_type] = vct;
                decl_to_vertex_layout_index[decl_vertex_type] = mesh->vertex_layout_size;

                Assert(mesh->vertex_layout_size < MAX_MESH_VERTEX_COMPONENTS);
                mesh->vertex_layout[mesh->vertex_layout_size] = vct;
                mesh->vertex_layout_size += 1;
                
                break;
            }
            case DECL_DATA_GO: {
                Assert(mesh->vertex_layout_size <= MAX_VERTEX_ARRAY_BINDINGS);

                mesh->vertex_size = 0;

                u32 offset = vertex_buffer_storage.size;
                Vertex_Array_Binding bindings[MAX_VERTEX_ARRAY_BINDINGS];
                s32 binding_count = 0;

                for (s32 i = 0; i < mesh->vertex_layout_size; ++i) {
                    const s32 vcs = get_vertex_component_size(mesh->vertex_layout[i]); 
                    mesh->vertex_size += vcs;

                    auto &binding = bindings[i];
                    binding.binding_index = i;
                    binding.data_offset = offset;
                    binding.layout_size = 1;
                    binding.layout[0] = { mesh->vertex_layout[i], 0 };
                    binding_count += 1;
                    
                    vertex_offsets[i] = offset;

                    const u32 size = vcs * mesh->vertex_count;
                    offset += size;
                }

                mesh->rid_vertex_array = r_create_vertex_array(bindings, binding_count);
                
                mesh->vertex_data_size   = mesh->vertex_size * mesh->vertex_count;
                mesh->vertex_data_offset = vertex_buffer_storage.size;

                vertex_buffer_storage.size += mesh->vertex_data_size;
                Assert(vertex_buffer_storage.size <= MAX_VERTEX_STORAGE_SIZE);

                mesh->index_data_size   = mesh->index_count  * sizeof(u32);
                mesh->index_data_offset = index_buffer_storage.size;

                index_buffer_storage.size += mesh->index_data_size;
                Assert(index_buffer_storage.size <= MAX_INDEX_STORAGE_SIZE);

                index_data_offset = mesh->index_data_offset;
                
                break;
            }
            case DECL_POSITION:
            case DECL_NORMAL:
            case DECL_UV: {
                const auto vct = decl_to_vct[decl_type];
                const s32 vli  = decl_to_vertex_layout_index[decl_type];
                
                if (decl_type == DECL_NORMAL) {
                    volatile int a = 0;
                }

                switch (vct) {
                case VERTEX_F32_2: {
                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case VERTEX_F32_3: {
                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case VERTEX_F32_4: {
                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    copy_bytes(r_vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                }
                
                break;
            }
            case DECL_INDEX: {
                char *sv = str_token(null, DELIMITERS);

                const u32 v = str_to_u32(sv);
                copy_bytes(r_index_data + index_data_offset, &v, sizeof(v));
                
                index_data_offset += sizeof(v);
                
                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

//#include <sstream>

static void init_mesh_asset_obj(Mesh *mesh, void *data) {
    char path[MAX_PATH_SIZE] = {'\0'};
    convert_to_full_asset_path(path, sid_str(mesh->sid_path));
    //std::istringstream sstream((char *)data);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string swarn;
    std::string serr;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &swarn, &serr, path, null, true);
    //bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &swarn, &serr, &sstream);

    if (!swarn.empty()) {
        warn("%s", swarn.c_str());
    }

    if (!serr.empty()) {
        error("%s", serr.c_str());
        return;
    }

    if (!ret) {
        error("Failed to load mesh %s", sid_str(mesh->sid_path));
        return;
    }

    mesh->vertex_layout_size = 0;
    
    if (attrib.vertices.size() > 0) {
        mesh->vertex_layout[mesh->vertex_layout_size] = VERTEX_F32_3;
        mesh->vertex_layout_size += 1;
    }

    if (attrib.normals.size() > 0) {
        mesh->vertex_layout[mesh->vertex_layout_size] = VERTEX_F32_3;
        mesh->vertex_layout_size += 1;
    }

    if (attrib.texcoords.size() > 0) {
        mesh->vertex_layout[mesh->vertex_layout_size] = VERTEX_F32_2;
        mesh->vertex_layout_size += 1;
    }

    mesh->vertex_count = 0;
    for (u32 s = 0; s < shapes.size(); s++) {
        for (u32 f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            mesh->vertex_count += (u32)shapes[s].mesh.num_face_vertices[f];
        }
    }

    u8 *r_vertex_data = (u8 *)vertex_buffer_storage.mapped_data;

    u32 vertex_offsets[MAX_MESH_VERTEX_COMPONENTS];
    u32 offset = vertex_buffer_storage.size;
    
    Vertex_Array_Binding bindings[MAX_VERTEX_ARRAY_BINDINGS];
    s32 binding_count = 0;

    for (s32 i = 0; i < mesh->vertex_layout_size; ++i) {
        const s32 vcs = get_vertex_component_size(mesh->vertex_layout[i]); 
        mesh->vertex_size += vcs;

        auto &binding = bindings[i];
        binding.binding_index = i;
        binding.data_offset = offset;
        binding.layout_size = 1;
        binding.layout[0] = { mesh->vertex_layout[i], 0 };
        binding_count += 1;
                    
        vertex_offsets[i] = offset;

        const u32 size = vcs * mesh->vertex_count;
        offset += size;
    }
                
    mesh->rid_vertex_array = r_create_vertex_array(bindings, binding_count);
                
    mesh->vertex_data_size   = mesh->vertex_size * mesh->vertex_count;
    mesh->vertex_data_offset = vertex_buffer_storage.size;

    vertex_buffer_storage.size += mesh->vertex_data_size;
    Assert(vertex_buffer_storage.size <= MAX_VERTEX_STORAGE_SIZE);

    for (u32 s = 0; s < shapes.size(); s++) {
        u32 index_offset = 0;
        for (u32 f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            const u32 fv = shapes[s].mesh.num_face_vertices[f];

            for (u32 v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                {
                    const f32 vx = attrib.vertices[3 * idx.vertex_index + 0];
                    const f32 vy = attrib.vertices[3 * idx.vertex_index + 1];
                    const f32 vz = attrib.vertices[3 * idx.vertex_index + 2];

                    const vec3 pos = vec3(vx, vy, vz);
                    copy_bytes(r_vertex_data + vertex_offsets[0], &pos, sizeof(pos));
                    vertex_offsets[0] += sizeof(pos);
                }
                
                if (idx.normal_index >= 0) {
                    const f32 nx = attrib.normals[3 * idx.normal_index + 0];
                    const f32 ny = attrib.normals[3 * idx.normal_index + 1];
                    const f32 nz = attrib.normals[3 * idx.normal_index + 2];

                    const vec3 n = vec3(nx, ny, nz);
                    copy_bytes(r_vertex_data + vertex_offsets[1], &n, sizeof(n));
                    vertex_offsets[1] += sizeof(n);
                }

                if (idx.texcoord_index >= 0) {
                    const f32 tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    const f32 ty = attrib.texcoords[2 * idx.texcoord_index + 1];

                    const vec2 uv = vec2(tx, ty);
                    copy_bytes(r_vertex_data + vertex_offsets[2], &uv, sizeof(uv));
                    vertex_offsets[2] += sizeof(uv);
                }
                
                // Optional: vertex colors
                // const f32 r = attrib.colors[3 * idx.vertex_index + 0];
                // const f32 g = attrib.colors[3 * idx.vertex_index + 1];
                // const f32 b = attrib.colors[3 * idx.vertex_index + 2];
            }

            index_offset += fv;

            // per-face material
            //shapes[s].mesh.material_ids[f];
        }
    }
}

void init_mesh_asset(Mesh *mesh, void *data) {
    // @Todo: parse mesh data - ideally text for editor and binary for release.

    const char *mesh_path = sid_str(mesh->sid_path);
    const char *extension = str_char_from_end(mesh_path, '.');
    if (str_cmp(extension, ".mesh")) {
        init_mesh_asset_mesh(mesh, data);
        return;
    } else if (str_cmp(extension, ".obj")) {
        init_mesh_asset_obj(mesh, data);
        return;
    }

    warn("Skipping mesh with unsupported format %s", mesh_path);
}

void init_material_asset(Material *material, void *data) {
    // @Todo: parse material data - ideally text for editor and binary for release.
        
    enum Declaration_Type {
        DECL_NONE,
        DECL_SHADER,
        DECL_TEXTURE,
        DECL_AMBIENT,
        DECL_DIFFUSE,
        DECL_SPECULAR,
        DECL_UNIFORM,
    };

    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;

    const char *DECL_NAME_SHADER   = "s";
    const char *DECL_NAME_TEXTURE  = "t";
    const char *DECL_NAME_AMBIENT  = "la";
    const char *DECL_NAME_DIFFUSE  = "ld";
    const char *DECL_NAME_SPECULAR = "ls";
    const char *DECL_NAME_UNIFORM  = "u";

    const char *DELIMITERS = " ";

    material->uniform_count = 0;
    
    char *p = (char *)data;
    p[material->data_size] = '\0';

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_SHADER)) {
                decl_type = DECL_SHADER;
            } else if (str_cmp(token, DECL_NAME_TEXTURE)) {
                decl_type = DECL_TEXTURE;
            } else if (str_cmp(token, DECL_NAME_AMBIENT)) {
                decl_type = DECL_AMBIENT;
            } else if (str_cmp(token, DECL_NAME_DIFFUSE)) {
                decl_type = DECL_DIFFUSE;
            } else if (str_cmp(token, DECL_NAME_SPECULAR)) {
                decl_type = DECL_SPECULAR;
            } else if (str_cmp(token, DECL_NAME_UNIFORM)) {
                decl_type = DECL_UNIFORM;
            } else {
                error("Unknown material token declaration '%s'", token);
                continue;
            }

            switch (decl_type) {
            case DECL_SHADER: {
                char *sv = str_token(null, DELIMITERS);
                material->sid_shader = sid_intern(sv);
                break;
            }
            case DECL_TEXTURE: {
                char *sv = str_token(null, DELIMITERS);
                material->sid_texture = sid_intern(sv);
                break;
            }
            case DECL_AMBIENT: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->ambient = v;
                break;
            }
            case DECL_DIFFUSE: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->diffuse = v;
                break;
            }
            case DECL_SPECULAR: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                material->specular = v;
                break;
            }
            case DECL_UNIFORM: {
                Assert(material->uniform_count < MAX_MATERIAL_UNIFORMS);

                auto &uniform = material->uniforms[material->uniform_count];
                uniform.count = 1;

                char *su_name = str_token(null, DELIMITERS);
                str_copy(uniform.name, su_name);

                char *su_type = str_token(null, DELIMITERS);
                if (str_cmp(su_type, TYPE_NAME_U32)) {
                    uniform.type = UNIFORM_U32;

                    u32 v = 0;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_u32(sv);
                    }
                    
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_F32)) {
                    uniform.type = UNIFORM_F32;

                    f32 v = 0.0f;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_f32(sv);
                    }
                    
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC2)) {
                    uniform.type = UNIFORM_F32_2;

                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }
                     
                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC3)) {
                    uniform.type = UNIFORM_F32_3;

                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_VEC4)) {
                    uniform.type = UNIFORM_F32_4;

                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else if (str_cmp(su_type, TYPE_NAME_MAT4)) {
                    uniform.type = UNIFORM_F32_4X4;

                    mat4 v = mat4_identity();
                    for (s32 i = 0; i < 16; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i % 4][i / 4] = str_to_f32(sv);
                    }

                    cache_uniform_value_on_cpu(&uniform, &v, sizeof(v));
                } else {
                    error("Unknown uniform type declaration '%s'", su_type);
                    continue;
                }

                material->uniform_count += 1;

                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}
