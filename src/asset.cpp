#include "pch.h"
#include "asset.h"
#include "log.h"
#include "profile.h"
#include "font.h"
#include "hash.h"
#include "hash_table.h"

#include "stb_image.h"
#include "tiny_obj_loader.h"

#include "os/file.h"
#include "os/time.h"

#include "audio/au_table.h"
#include "audio/au_sound.h"
#include "audio/wav.h"

#include "render/render.h"
#include "render/r_table.h"
#include "render/r_storage.h"
#include "render/r_shader.h"
#include "render/r_texture.h"
#include "render/r_material.h"
#include "render/r_uniform.h"
#include "render/r_mesh.h"
#include "render/r_vertex_descriptor.h"
#include "render/r_flip_book.h"

#include "math/matrix.h"

const String TYPE_NAME_U32  = S("u32");
const String TYPE_NAME_F32  = S("f32");
const String TYPE_NAME_VEC2 = S("vec2");
const String TYPE_NAME_VEC3 = S("vec3");
const String TYPE_NAME_VEC4 = S("vec4");
const String TYPE_NAME_MAT4 = S("mat4");

struct Asset_Source_Callback_Data {
    Asset_Type asset_type = ASSET_NONE;
    String filter_ext;
};

static void init_asset_source_callback(const File_Callback_Data *data) {
    auto *ascd = (Asset_Source_Callback_Data *)data->user_data;
    const String ext = str_from(data->path, str_index(data->path, '.', S_SEARCH_REVERSE_BIT));
    
    if (is_valid(ascd->filter_ext) && !str_equal(ascd->filter_ext, ext)) {
        return;
    }

    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    String full_path = str_copy(scratch.arena, data->path);
    fix_directory_delimiters(full_path);

    const String relative_path = to_relative_asset_path(scratch.arena, full_path);
    const sid sid_relative_path = sid_intern(relative_path);
    
    Asset_Source source;
    source.asset_type = ascd->asset_type;
    source.last_write_time = data->last_write_time;
    source.sid_full_path = sid_intern(full_path);
    source.sid_relative_path = sid_relative_path;
    
    auto &ast = Asset_source_table;
    table_push(ast.table, sid_relative_path, source);
    ast.count_by_type[source.asset_type] += 1;
}

static inline void init_asset_sources(const char *path, Asset_Type type, const String &filter_ext = STRING_NONE) {
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
    
    auto &ast = Asset_source_table;

    constexpr f32 SCALE = 2.0f - ast.table.MAX_LOAD_FACTOR;
    constexpr u32 COUNT = (u32)(Asset_source_table.MAX_COUNT * SCALE);
    
    // @Cleanup: use own arena?
    table_reserve(M_global, ast.table, COUNT);
    table_custom_hash(ast.table, &asset_source_table_hash);
    mem_set(ast.count_by_type, 0, sizeof(ast.count_by_type));
    
    init_asset_sources(DIR_SHADERS,    ASSET_SHADER, SHADER_FILE_EXT);
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
    constexpr f32 SCALE = 2.0f - Asset_table.MAX_LOAD_FACTOR;
    constexpr u32 COUNT = (u32)(Asset_source_table.MAX_COUNT * SCALE);

    // @Cleanup: use own arena?
    table_reserve(M_global, Asset_table, COUNT);
    table_custom_hash(Asset_table, &asset_table_hash);
}

Asset *find_asset(sid path) {
    return table_find(Asset_table, path);
}

R_Shader *find_shader(sid path) {
    auto *asset = find_asset(path);
    if (asset == null) return null;
    return &R_table.shaders[asset->index];
}

R_Texture *find_texture(sid path) {
    auto *asset = find_asset(path);
    if (asset == null) return null;
    return &R_table.textures[asset->index];
}

R_Material *find_material(sid path) {
    auto *asset = find_asset(path);
    if (asset == null) return null;
    return &R_table.materials[asset->index];
}

R_Mesh *find_mesh(sid path) {
    auto *asset = find_asset(path);
    if (asset == null) return null;
    return &R_table.meshes[asset->index];
}

static void parse_asset_file_data(String contents, R_Material::Meta &meta,
                                  u16 umeta_count, R_Uniform::Meta *umetas) {
    constexpr String DECL_SHADER   = S("s");
    constexpr String DECL_TEXTURE  = S("t");
    constexpr String DECL_AMBIENT  = S("la");
    constexpr String DECL_DIFFUSE  = S("ld");
    constexpr String DECL_SPECULAR = S("ls");
    constexpr String DECL_UNIFORM  = S("u");
    constexpr String DELIMITERS    = S(" \r\n\t");

    meta.uniform_count = 0;
    
    String t = contents;
    String line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);
    String_Token_Iterator it = { line, DELIMITERS, 0 };
    
    while (is_valid(line)) {
        String token = str_token(it);
        
        if (is_valid(token)) {
            if (str_equal(token, DECL_SHADER)) {
                const String path = str_token(it);
                if (is_valid(path)) {
                    meta.shader = sid_intern(path);
                } else {
                    error("Failed to find shader path");
                }
            } else if (str_equal(token, DECL_TEXTURE)) {
                const String path = str_token(it);
                if (is_valid(path)) {
                    meta.texture = sid_intern(path);
                } else {
                    error("Failed to find texture path");
                }
            } else if (str_equal(token, DECL_AMBIENT)) {
                for (s32 i = 0; i < 3; ++i) {
                    const String s = str_token(it);
                    if (!is_valid(s)) break;
                    meta.light_params.ambient[i] = str_to_f32(s);
                }
            } else if (str_equal(token, DECL_DIFFUSE)) {
                for (s32 i = 0; i < 3; ++i) {
                    const String s = str_token(it);
                    if (!is_valid(s)) break;
                    meta.light_params.diffuse[i] = str_to_f32(s);
                }
            } else if (str_equal(token, DECL_SPECULAR)) {
                for (s32 i = 0; i < 3; ++i) {
                    const String s = str_token(it);
                    if (!is_valid(s)) break;
                    meta.light_params.specular[i] = str_to_f32(s);
                }
            } else if (str_equal(token, DECL_UNIFORM)) {
                // @Todo: handle error instead of assert.
                Assert(meta.uniform_count < umeta_count);

                auto &umeta = umetas[meta.uniform_count];
                umeta.count = 1;

                const String su_name = str_token(it);
                umeta.name = sid_intern(su_name);

                bool correct_uniform = true;
                const String su_type = str_token(it);
                if (str_equal(su_type, TYPE_NAME_U32)) {
                    umeta.type = R_U32;
                } else if (str_equal(su_type, TYPE_NAME_F32)) {
                    umeta.type = R_F32;
                } else if (str_equal(su_type, TYPE_NAME_VEC2)) {
                    umeta.type = R_F32_2;
                } else if (str_equal(su_type, TYPE_NAME_VEC3)) {
                    umeta.type = R_F32_3;
                } else if (str_equal(su_type, TYPE_NAME_VEC4)) {
                    umeta.type = R_F32_4;
                } else if (str_equal(su_type, TYPE_NAME_MAT4)) {
                    umeta.type = R_F32_4X4;
                } else {
                    error("Unknown uniform type declaration '%s'", su_type);
                    correct_uniform = false;
                }

                if (correct_uniform) {
                    meta.uniform_count += 1;
                }
            } else {
                error("Unknown material token declaration %.*s", token.length, token.value);
            }
        }

        t    = str_slice(t, ASCII_NEW_LINE, S_INDEX_PLUS_ONE_BIT);
        line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);

        // @Note: string token iterator holds offset to its string, but as we update
        // the line string (thus its not static) we need to reset iterator position.
        it.pos = 0;
    }
}

static void parse_asset_file_data(String contents, R_Flip_Book::Meta &meta) {
    constexpr String DECL_TEXTURE    = S("t");
    constexpr String DECL_FRAME_TIME = S("ft");
    constexpr String DELIMITERS      = S(" \r\n\t");
    
    meta.count = 0;

    String t = contents;
    String line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);
    String_Token_Iterator it = { line, DELIMITERS, 0 };
    
    while (is_valid(line)) {
        String token = str_token(it);
        
        if (is_valid(token)) {
            if (str_equal(token, DECL_TEXTURE)) {
                const String path = str_token(it);

                if (is_valid(path)) {
                    Assert(meta.count < R_Flip_Book::MAX_FRAMES);
                    meta.textures[meta.count] = sid_intern(path);
                    meta.count += 1;
                } else {
                    error("Failed to find frame texture path %d", meta.count);
                }
            } else if (str_equal(token, DECL_FRAME_TIME)) {
                const String s = str_token(it);

                if (is_valid(s)) {
                    const f32 v = str_to_f32(s);
                    meta.next_frame_time = v;
                } else {
                    error("Failed to find frame time value");
                }
            } else {
                error("Unknown flip book token %.*s", token.length, token.value);
            }
        }
        
        t    = str_slice(t, ASCII_NEW_LINE, S_INDEX_PLUS_ONE_BIT);
        line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);

        // @Note: string token iterator holds offset to its string, but as we update
        // the line string (thus its not static) we need to reset iterator position.
        it.pos = 0;
    }
}

static void parse_asset_file_data_mesh(String contents, R_Mesh::Meta &meta,
                                       u16 components_count, u16 *components,
                                       u32 vertices_size, void *vertices,
                                       u32 indices_size, void *indices)  {
    constexpr u32 DECL_COUNT = 8;
    
    constexpr String DECL_VERTEX_COUNT     = S("vn");
    constexpr String DECL_INDEX_COUNT      = S("in");
    constexpr String DECL_VERTEX_COMPONENT = S("vc");
    constexpr String DECL_DATA_GO          = S("go");
    constexpr String DECL_POSITION         = S("p");
    constexpr String DECL_NORMAL           = S("n");
    constexpr String DECL_UV               = S("uv");
    constexpr String DECL_INDEX            = S("i");
    constexpr String DELIMITERS            = S(" \r\n\t");

    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    meta = {};

    u32 index_offset = 0;
    u32 vertex_offsets[R_Vertex_Descriptor::MAX_BINDINGS];
    
    struct Decl_Table_Value { u16 vct; u16 component_index; };
    Table<String, Decl_Table_Value> decl_table;
    table_reserve(scratch.arena, decl_table, DECL_COUNT * 2);
    table_custom_hash   (decl_table, [] (const String &a) { return hash_fnv(a); });
    table_custom_compare(decl_table, [] (const String &a, const String &b) { return str_equal(a, b); });
    
    String t = contents;
    String line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);
    String_Token_Iterator it = { line, DELIMITERS, 0 };
    
    while (is_valid(line)) {
        String token = str_token(it);

        if (is_valid(token)) {
            if (str_equal(token, DECL_VERTEX_COUNT)) {
                String s = str_token(it);
                if (is_valid(s)) {
                    meta.vertex_count = str_to_u32(s);
                } else {
                    error("Failed to find vertex count");
                }
            } else if (str_equal(token, DECL_INDEX_COUNT)) {
                String s = str_token(it);
                if (is_valid(s)) {
                    meta.index_count = str_to_u32(s);
                } else {
                    error("Failed to find index count");
                }
            } else if (str_equal(token, DECL_VERTEX_COMPONENT)) {
                String sdecl = str_token(it);

                bool correct_vc = true;
                if (str_equal(sdecl, DECL_POSITION)
                    || str_equal(sdecl, DECL_NORMAL)
                    || str_equal(sdecl, DECL_UV)) {
                    String stype = str_token(it);

                    u16 vct = R_NONE;
                    if (str_equal(stype, TYPE_NAME_VEC2)) {
                        vct = R_F32_2;
                    } else if (str_equal(stype, TYPE_NAME_VEC3)) {
                        vct = R_F32_3;
                    } else if (str_equal(stype, TYPE_NAME_VEC4)) {
                        vct = R_F32_4;
                    } else {
                        error("Unknown mesh vertex component type token %.*s",
                              token.length, token.value);
                    }

                    if (vct != R_NONE) {
                        Assert(meta.vertex_component_count < components_count);
                        components[meta.vertex_component_count] = vct;

                        table_push(decl_table, sdecl, { vct, meta.vertex_component_count });
                        
                        meta.vertex_component_count += 1; 
                    }
                } else {
                    error("Unknown mesh vertex component token %.*s",
                          token.length, token.value);
                }
            } else if (str_equal(token, DECL_DATA_GO)) {                
                u32 offset = 0;
                for (u32 i = 0; i < meta.vertex_component_count; ++i) {
                    const u16 vcs = r_vertex_component_size(components[i]); 
                    meta.vertex_size += vcs;
                    
                    vertex_offsets[i] = offset;

                    const u32 size = vcs * meta.vertex_count;
                    offset += size;
                }

                // @Todo: handle error instead of macro, maybe take arena to alloc from
                // instead of given buffers.
                Assert(vertices_size >= meta.vertex_count * meta.vertex_size);
                Assert(indices_size  >= meta.index_count  * sizeof(u32));
            } else if (str_equal(token, DECL_POSITION)
                       || str_equal(token, DECL_NORMAL)
                       || str_equal(token, DECL_UV)) {
                const auto *table_value = table_find(decl_table, token);
                if (!table_value) {
                    error("Failed to find decl data from decl table 0x%X by token %.*s",
                          &decl_table, token.length, token.value);
                } else {
                    const u16 vct = table_value->vct;
                    const u16 vci = table_value->component_index;

                    u16 dimension = 0;
                    switch (vct) {
                    case R_F32_2: { dimension = 2; break; }
                    case R_F32_3: { dimension = 3; break; }
                    case R_F32_4: { dimension = 4; break; }
                    default: {
                        error("Unknown vertex component type %d from decl table 0x%X",
                              vct, &decl_table);
                        break;
                    }
                    }

                    if (dimension > 0) {
                        Assert(dimension < 16);

                        f32 v[16];
                        for (u16 i = 0; i < dimension; ++i) {
                            String s = str_token(it);
                            v[i] = str_to_f32(s);
                        }

                        const u32 offset = vertex_offsets[vci];
                        const u32 size   = dimension * sizeof(f32);
                        
                        mem_copy((u8 *)vertices + offset, &v, size);
                        vertex_offsets[vci] += size;
                    } else {
                        error("Wrong dimension %d of vertex component %d", dimension, vct);
                    }
                }
            } else if (str_equal(token, DECL_INDEX)) {
                String s = str_token(it);

                if (is_valid(s)) {
                    const u32 v = str_to_u32(s);
                    mem_copy((u8 *)indices + index_offset, &v, sizeof(v));
                    index_offset += sizeof(v);
                } else {
                    error("Failed to find index value");
                }
            } else {
                error("Unknown mesh token %.*s", token.length, token.value);
            }
        }
        
        t    = str_slice(t, ASCII_NEW_LINE, S_INDEX_PLUS_ONE_BIT);
        line = str_slice(t, ASCII_NEW_LINE, S_LEFT_SLICE_BIT);

        // @Note: string token iterator holds offset to its string, but as we update
        // the line string (thus its not static) we need to reset iterator position.
        it.pos = 0;
    }
}

#include <sstream>

static void parse_asset_file_data_obj(String contents, R_Mesh::Meta &meta,
                                      u16 components_count, u16 *components,
                                      u32 vertices_size, void *vertices,
                                      u32 indices_size, void *indices) {
    const auto sdata = std::string(contents.value, contents.length);
    std::istringstream sstream(sdata);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string swarn;
    std::string serr;

    tinyobj::MaterialFileReader mat_file_reader(DIR_MATERIALS);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &swarn, &serr, &sstream, &mat_file_reader);

    if (!swarn.empty()) {
        warn("%s", swarn.c_str());
    }

    if (!serr.empty()) {
        error("%s", serr.c_str());
        return;
    }

    if (!ret) {
        error("Failed to load mesh");
        return;
    }

    meta = {};
        
    if (attrib.vertices.size() > 0) {
        Assert(meta.vertex_component_count < components_count);
        components[meta.vertex_component_count] = R_F32_3;
        meta.vertex_component_count += 1;
    }

    if (attrib.normals.size() > 0) {
        Assert(meta.vertex_component_count < components_count);
        components[meta.vertex_component_count] = R_F32_3;
        meta.vertex_component_count += 1;
    }

    if (attrib.texcoords.size() > 0) {
        Assert(meta.vertex_component_count < components_count);
        components[meta.vertex_component_count] = R_F32_2;
        meta.vertex_component_count += 1;
    }
                
    for (u32 s = 0; s < shapes.size(); s++) {
        for (u32 f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            meta.vertex_count += (u32)shapes[s].mesh.num_face_vertices[f];
        }
    }

    u32 offset = 0;
    u32 vertex_offsets[R_Vertex_Descriptor::MAX_BINDINGS];
    
    for (u16 i = 0; i < meta.vertex_component_count; ++i) {
        const u16 vcs = r_vertex_component_size(components[i]); 
        meta.vertex_size += vcs;
                    
        vertex_offsets[i] = offset;

        const u32 size = vcs * meta.vertex_count;
        offset += size;
    }

    const u32 vertex_data_size = meta.vertex_size * meta.vertex_count;
    Assert(vertex_data_size <= vertices_size);
    
    u8 *vertex_data = (u8 *)vertices;
    
    // @Todo: parse indices as well, as now all vertices are collected which is a bit dumb.
    
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

                    const vec3 pos = vec3{vx, vy, vz};
                    mem_copy(vertex_data + vertex_offsets[0], &pos, sizeof(pos));
                    vertex_offsets[0] += sizeof(pos);
                }
                
                if (idx.normal_index >= 0) {
                    const f32 nx = attrib.normals[3 * idx.normal_index + 0];
                    const f32 ny = attrib.normals[3 * idx.normal_index + 1];
                    const f32 nz = attrib.normals[3 * idx.normal_index + 2];

                    const vec3 n = vec3{nx, ny, nz};
                    mem_copy(vertex_data + vertex_offsets[1], &n, sizeof(n));
                    vertex_offsets[1] += sizeof(n);
                }

                if (idx.texcoord_index >= 0) {
                    const f32 tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    const f32 ty = attrib.texcoords[2 * idx.texcoord_index + 1];

                    const vec2 uv = vec2{tx, ty};
                    mem_copy(vertex_data + vertex_offsets[2], &uv, sizeof(uv));
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

u32 get_asset_max_file_size(Asset_Type type) {
    switch (type) {
    case ASSET_SHADER:    return R_Shader::MAX_FILE_SIZE;
    case ASSET_TEXTURE:   return R_Texture::MAX_FILE_SIZE;
    case ASSET_MATERIAL:  return R_Material::MAX_FILE_SIZE;
    case ASSET_MESH:      return R_Mesh::MAX_FILE_SIZE;
    case ASSET_FONT:      return Font_Info::MAX_FILE_SIZE;
    case ASSET_FLIP_BOOK: return R_Flip_Book::MAX_FILE_SIZE;
    case ASSET_SOUND:     return Au_Sound::MAX_FILE_SIZE;
    default: return 0;
    }
}

u32 get_asset_meta_size(Asset_Type type) {
    switch (type) {
    case ASSET_SHADER:    return 0;
    case ASSET_TEXTURE:   return sizeof(R_Texture::Meta);
    case ASSET_MATERIAL:  return sizeof(R_Material::Meta);
    case ASSET_MESH:      return sizeof(R_Mesh::Meta);
    case ASSET_FONT:      return 0;
    case ASSET_FLIP_BOOK: return sizeof(R_Flip_Book::Meta);
    case ASSET_SOUND:     return sizeof(Au_Sound::Meta);
    default: return 0;
    }
}

static void *get_asset_meta(Asset_Type type) {
    switch (type) {
    case ASSET_SHADER: return null;
    case ASSET_TEXTURE: {
        static R_Texture::Meta meta;
        return &meta;
    }
    case ASSET_MATERIAL: {
        static R_Material::Meta meta;
        return &meta;
    }
    case ASSET_MESH: {
        static R_Mesh::Meta meta;
        return &meta;
    }
    case ASSET_FLIP_BOOK: {
        static R_Flip_Book::Meta meta;
        return &meta;
    } 
    case ASSET_SOUND: {
        static Au_Sound::Meta meta;
        return &meta;
    }
    default: return null;
    }
}

void serialize(File file, Asset &asset) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    u32 meta_size = get_asset_meta_size(asset.type);
    void *meta = get_asset_meta(asset.type);

    const String relative_path = sid_str(asset.path);
    const String path = to_full_asset_path(scratch.arena, relative_path);

    Buffer contents = os_read_file(scratch.arena, path);
    
    switch (asset.type) {
    case ASSET_SHADER: {
        String s = parse_shader_includes(scratch.arena, String { (char *)contents.data, contents.size });

        contents.data = (u8 *)s.value;
        contents.size = s.length;
        
        break;
    }
    case ASSET_TEXTURE: {
        meta_size = 0;        
        break;
    }
    case ASSET_MATERIAL: {
        String s = String { (char *)contents.data, contents.size };

        auto &mmeta = *(R_Material::Meta *)meta;
        R_Uniform::Meta umetas[R_Material::MAX_UNIFORMS] = { 0 };
        parse_asset_file_data(s, mmeta, COUNT(umetas), umetas);

        contents.size = mmeta.uniform_count * sizeof(umetas[0]);
        mem_copy(contents.data, umetas, contents.size);
        
        break;
    }
    case ASSET_MESH: {
        constexpr u32 VERTICES_SIZE = MB(16);
        constexpr u32 INDICES_SIZE  = MB(4);

        Scratch scratch = local_scratch();
        defer { release(scratch); };
        
        String s = String { (char *)contents.data, contents.size };

        auto &mmeta = *(R_Mesh::Meta *)meta;

        u16 components[R_Vertex_Binding::MAX_COMPONENTS];

        void *vertices = push(scratch.arena, VERTICES_SIZE);
        void *indices  = push(scratch.arena, INDICES_SIZE);
        
        const String extension = str_slice(path, '.', S_SEARCH_REVERSE_BIT);
        if (str_equal(extension, S(".mesh"))) {
            parse_asset_file_data_mesh(s, mmeta, COUNT(components), components,
                                       VERTICES_SIZE, vertices, INDICES_SIZE, indices);
        } else if (str_equal(extension, S(".obj"))) {
            parse_asset_file_data_obj(s, mmeta, COUNT(components), components,
                                      VERTICES_SIZE, vertices, INDICES_SIZE, indices);
        }

        const u32 comps_size    = mmeta.vertex_component_count * sizeof(components[0]);
        const u32 vertices_size = mmeta.vertex_count * mmeta.vertex_size;
        const u32 indices_size  = mmeta.index_count  * sizeof(u32);

        u8 *dst = contents.data;
        u64 write_size = 0;
        
        mem_copy(dst + write_size, components, comps_size);    write_size += comps_size;
        mem_copy(dst + write_size, vertices,   vertices_size); write_size += vertices_size;
        mem_copy(dst + write_size, indices,    indices_size);  write_size += indices_size;
        
        contents.size = write_size;
        
        break;
    }
    case ASSET_FLIP_BOOK: {
        String s = String { (char *)contents.data, contents.size };

        contents.size = 0;

        auto &mmeta = *(R_Flip_Book::Meta *)meta;
        parse_asset_file_data(s, mmeta);

        break;
    }
    case ASSET_SOUND: {
        auto &smeta = *(Au_Sound::Meta *)meta;

        Wav_Header wav;
        contents.data = (u8 *)parse_wav(contents.data, &wav);

        smeta.channel_count = wav.channel_count;
        smeta.sample_rate   = wav.samples_per_second;
        smeta.bit_rate      = wav.bits_per_sample;

        contents.size = wav.sampled_data_size;

        break;
    }
    }
    
    asset.index = 0;
    asset.pak_meta_size = meta_size;
    asset.pak_data_size = (u32)contents.size;
    
    os_write_file(file, _sizeref(asset));

    os_set_file_ptr(file, asset.pak_blob_offset);
    os_write_file(file, meta_size, meta);
    os_write_file(file, contents.size, contents.data);
}

void deserialize(File file, Asset &asset) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    os_read_file(file, _sizeref(asset));

    os_set_file_ptr(file, asset.pak_blob_offset);

    void *meta = push(scratch.arena, asset.pak_meta_size);
    void *data = push(scratch.arena, asset.pak_data_size);

    os_read_file(file, asset.pak_meta_size, meta);
    os_read_file(file, asset.pak_data_size, data);
    
    switch (asset.type) {
    case ASSET_SHADER: {
        asset.index = r_create_shader(String { (char *)data, asset.pak_data_size });
        break;
    }
    case ASSET_TEXTURE: {
        s32 w, h, cc;
        data = stbi_load_from_memory((u8 *)data, (s32)asset.pak_data_size, &w, &h, &cc, 0);
        
        const u16 format = r_format_from_channel_count(cc);

        // @Cleanup: temp default values, not really fine.
        const u16 type = R_TEXTURE_2D;
        const u16 wrap = R_REPEAT;
        const u16 min_filter = R_NEAREST_MIPMAP_LINEAR;
        const u16 mag_filter = R_NEAREST;
        
        asset.index = r_create_texture(type, format, w, h,
                                       wrap, min_filter, mag_filter, data);
        break;
    }
    case ASSET_MATERIAL: {
        const auto &mmeta = *(R_Material::Meta *)meta;

        const auto shader  = Asset_table[mmeta.shader].index;
        const auto texture = Asset_table[mmeta.texture].index;

        const auto *umetas = (R_Uniform::Meta *)data;
        
        u16 uniforms[R_Material::MAX_UNIFORMS] = { 0 };
        for (u16 i = 0; i < mmeta.uniform_count; ++i) {
            const auto &umeta = umetas[i];
            uniforms[i] = r_create_uniform(umeta.name, umeta.type, umeta.count);
        }
        
        asset.index = r_create_material(shader, texture, mmeta.light_params,
                                        mmeta.uniform_count, uniforms);

        break;
    }
    case ASSET_MESH: {
        const auto &mmeta = *(R_Mesh::Meta *)meta;
        Assert(mmeta.vertex_component_count <= R_Vertex_Binding::MAX_COMPONENTS);

        const u32 vertex_components_size = mmeta.vertex_component_count * sizeof(u16);
        const u32 vertices_size = mmeta.vertex_count * mmeta.vertex_size;
        const u32 indices_size = mmeta.index_count * sizeof(u32);

        const u16 *vertex_components = (u16 *)data;
        const void *vertices = (u8 *)data + vertex_components_size; 
        const void *indices = (u8 *)data + vertex_components_size + vertices_size; 
        
        u32 vertices_offset = 0;

        R_Vertex_Binding bindings[R_Vertex_Descriptor::MAX_BINDINGS];
        u32 binding_count = 0;

        for (u32 i = 0; i < mmeta.vertex_component_count; ++i) {
            const u16 vcs = r_vertex_component_size(vertex_components[i]); 
            const u32 size = vcs * mmeta.vertex_count;
            const auto alloc = r_alloc(R_vertex_map_range, size);

            mem_copy(alloc.data, (u8 *)vertices + vertices_offset, size);
            vertices_offset += size;
                        
            auto &binding = bindings[i];
            binding.binding_index = i;
            binding.offset = alloc.map->offset + alloc.offset;
            binding.component_count = 1;
            binding.components[0] = { vertex_components[i], 0 };

            binding_count += 1;
        }

        const auto ialloc = r_alloc(R_index_map_range, indices_size);
        mem_copy(ialloc.data, indices, indices_size);
        
        const u16 vertex_desc = r_create_vertex_descriptor(binding_count, bindings);
        const u32 first_index = (ialloc.map->offset + ialloc.offset) / sizeof(u32);
        asset.index = r_create_mesh(vertex_desc, mmeta.vertex_count,
                                    first_index, mmeta.index_count);
        
        break;
    }
    case ASSET_FONT: {
        auto &font_info = Font_infos[Font_info_count];

        // @Cleanup: for now we store font data in global arena, this will be removed
        // after we add font atlas offline bake.
        void *font_data = push(M_global, asset.pak_data_size);
        mem_copy(font_data, data, asset.pak_data_size);
        
        init_font(font_data, font_info);

        asset.index = Font_info_count;
        Font_info_count += 1;

        break;
    }
    case ASSET_FLIP_BOOK: {
        const auto &fmeta = *(R_Flip_Book::Meta *)meta;

        u16 textures[R_Flip_Book::MAX_FRAMES];
        for (u32 i = 0; i < fmeta.count; ++i) {
            textures[i] = find_asset(fmeta.textures[i])->index;
        }
        
        asset.index = r_create_flip_book(fmeta.count, textures, fmeta.next_frame_time);

        break;
    }
    case ASSET_SOUND: {
        auto &smeta = *(Au_Sound::Meta *)meta;
        asset.index = au_create_sound(smeta.channel_count, smeta.bit_rate,
                                      smeta.sample_rate, asset.pak_data_size, data, 0);
        break;
    }
    }
}

static inline u32 get_asset_blob_size(const Asset &asset) {
    return asset.pak_meta_size + asset.pak_data_size;
}

void save_asset_pack(String path) {
    START_SCOPE_TIMER(save);

    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_WRITE_BIT);
    defer { os_close_file(file); };
    
    if (file == FILE_NONE) {
        log("Asset pak %s does not exist, creating new one", path);
        file = os_open_file(path, FILE_OPEN_NEW, FILE_WRITE_BIT);
        if (file == FILE_NONE) {
            error("Failed to create new asset pak %s", path);
            return;
        }
    }

    auto &ast = Asset_source_table;

    Asset_Pak_Header header;
    header.magic   = ASSET_PAK_MAGIC;
    header.version = ASSET_PAK_VERSION;

    u64 offsets[ASSET_TYPE_COUNT];
    u64 offset = sizeof(header);
    
    for (u8 i = 0; i < ASSET_TYPE_COUNT; ++i) {
        offsets[i] = offset;
        header.counts[i]  = ast.count_by_type[i];
        header.offsets[i] = offset;
        offset += sizeof(Asset) * ast.count_by_type[i];
    }

    os_write_file(file, _sizeref(header));

    For (ast.table) {
        String spath = sid_str(it.value.sid_relative_path);
        
        Asset asset;
        asset.type = it.value.asset_type;
        asset.path = it.value.sid_relative_path;
        asset.pak_blob_offset = offset;

        os_set_file_ptr(file, offsets[asset.type]);
        serialize(file, asset);

        offsets[asset.type] += sizeof(Asset);
        offset += get_asset_blob_size(asset);
    }
    
    log("Saved asset pack %.*s in %.2fms",
        path.length, path.value, CHECK_SCOPE_TIMER_MS(save));
}

void load_asset_pack(String path) {
    START_SCOPE_TIMER(load);

    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_READ_BIT);
    defer { os_close_file(file); };
    
    if (file == FILE_NONE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    Asset_Pak_Header header;
    os_read_file(file, _sizeref(header));

    if (header.magic != ASSET_PAK_MAGIC) {
        error("Wrong asset pak %s magic %u", path, header.magic);
        return;
    }

    if (header.version != ASSET_PAK_VERSION) {
        error("Wrong asset pak %s version %d", path, header.version);
        return;
    }

    for (u8 i = 0; i < ASSET_TYPE_COUNT; ++i) {            
        const auto type  = (Asset_Type)i;
        const u32 count  = header.counts[i];
        const u64 offset = header.offsets[i];
        const u32 size   = sizeof(Asset);

        for (u32 j = 0; j < count; ++j) {
            os_set_file_ptr(file, offset + size * j);

            Asset asset;
            deserialize(file, asset);

            table_push(Asset_table, asset.path, asset);
        }
    }
    
    log("Loaded asset pack %.*s in %.2fms",
        path.length, path.value, CHECK_SCOPE_TIMER_MS(load));
}

String to_relative_asset_path(Arena &a, String path) {
    String data = str_slice(path, S("\\data"));
    if (!is_valid(data)) data = str_slice(path, S("/data"));
    if (!is_valid(data)) return STRING_NONE;

    String_Builder sb;
    str_build(a, sb, data);

    String s = str_build_finish(a, sb);
    fix_directory_delimiters(s);

    return s;
}

String to_full_asset_path(Arena &a, String path) {
    String_Builder sb;
    str_build(a, sb, DIR_RUN_TREE);
    str_build(a, sb, path);

    String s = str_build_finish(a, sb);
    fix_directory_delimiters(s);

    return s;
}
