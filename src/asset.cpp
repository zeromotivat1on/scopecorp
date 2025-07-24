#include "pch.h"
#include "asset.h"
#include "log.h"
#include "sid.h"
#include "str.h"
#include "profile.h"
#include "font.h"

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
        return;
    }

    char full_path[MAX_PATH_SIZE];
    str_copy(full_path, data->path);
    fix_directory_delimiters(full_path);

    char relative_path[MAX_PATH_SIZE];
    to_relative_asset_path(relative_path, full_path);

    const sid sid_relative_path = sid_intern(relative_path);
    
    Asset_Source source;
    source.asset_type = ascd->asset_type;
    source.last_write_time = data->last_write_time;
    source.sid_full_path = sid_intern(full_path);
    source.sid_relative_path = sid_relative_path;
    
    auto &ast = Asset_source_table;
    add(ast.table, sid_relative_path, source);
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
    constexpr f32 SCALE = 2.0f - MAX_HASH_TABLE_LOAD_FACTOR;
    constexpr u32 COUNT = (u32)(Asset_source_table.MAX_COUNT * SCALE);

    START_SCOPE_TIMER(init);

    auto &ast = Asset_source_table;
    
    ast.table = Hash_Table<sid, Asset_Source>(COUNT);
    ast.table.hash_function = &asset_source_table_hash;
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
    constexpr f32 SCALE = 2.0f - MAX_HASH_TABLE_LOAD_FACTOR;
    constexpr u32 COUNT = (u32)(Asset_source_table.MAX_COUNT * SCALE);
        
    Asset_table = Asset_Table(COUNT);
    Asset_table.hash_function = &asset_table_hash;
}

Asset *find_asset(sid path) {
    return find(Asset_table, path);
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

static void parse_asset_file_data(const void *data, R_Material::Meta &meta,
                                  u16 umeta_count, R_Uniform::Meta *umetas) {
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

    meta.uniform_count = 0;
    
    const char *p = (const char *)data;

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    const char *new_line = str_char(p, ASCII_NEW_LINE);
    
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
                meta.shader = sid_intern(sv);
                break;
            }
            case DECL_TEXTURE: {
                char *sv = str_token(null, DELIMITERS);
                meta.texture = sid_intern(sv);
                break;
            }
            case DECL_AMBIENT: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                meta.light_params.ambient = v;
                break;
            }
            case DECL_DIFFUSE: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                meta.light_params.diffuse = v;
                break;
            }
            case DECL_SPECULAR: {
                vec3 v;
                for (s32 i = 0; i < 3; ++i) {
                    char *sv = str_token(null, DELIMITERS);
                    if (!sv) break;
                    v[i] = str_to_f32(sv);
                }

                meta.light_params.specular = v;
                break;
            }
            case DECL_UNIFORM: {
                Assert(meta.uniform_count < umeta_count);

                auto &umeta = umetas[meta.uniform_count];
                umeta.count = 1;

                char *su_name = str_token(null, DELIMITERS);
                umeta.name = sid_intern(su_name);

                char *su_type = str_token(null, DELIMITERS);
                if (str_cmp(su_type, TYPE_NAME_U32)) {
                    umeta.type = R_U32;

                    u32 v = 0;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_u32(sv);
                    }
                } else if (str_cmp(su_type, TYPE_NAME_F32)) {
                    umeta.type = R_F32;

                    f32 v = 0.0f;
                    char *sv = str_token(null, DELIMITERS);
                    if (sv) {
                        v = str_to_f32(sv);
                    }
                } else if (str_cmp(su_type, TYPE_NAME_VEC2)) {
                    umeta.type = R_F32_2;

                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }
                } else if (str_cmp(su_type, TYPE_NAME_VEC3)) {
                    umeta.type = R_F32_3;

                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }
                } else if (str_cmp(su_type, TYPE_NAME_VEC4)) {
                    umeta.type = R_F32_4;

                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i] = str_to_f32(sv);
                    }
                } else if (str_cmp(su_type, TYPE_NAME_MAT4)) {
                    umeta.type = R_F32_4X4;

                    mat4 v = mat4_identity();
                    for (s32 i = 0; i < 16; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        if (!sv) break;
                        v[i % 4][i / 4] = str_to_f32(sv);
                    }
                } else {
                    error("Unknown uniform type declaration '%s'", su_type);
                    continue;
                }

                meta.uniform_count += 1;

                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

static void parse_asset_file_data(const void *data, R_Flip_Book::Meta &meta) {
    enum Declaration_Type {
        DECL_NONE,
        DECL_TEXTURE,
        DECL_FRAME_TIME,
    };

    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;

    const char *DECL_NAME_TEXTURE = "t";
    const char *DECL_NAME_FRAME_TIME = "ft";

    const char *DELIMITERS = " ";
    
    meta.count = 0;
    
    const char *p = (const char *)data;
    
    char line_buffer[MAX_LINE_BUFFER_SIZE];
    const char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_TEXTURE)) {
                decl_type = DECL_TEXTURE;
            } else if (str_cmp(token, DECL_NAME_FRAME_TIME)) {
                decl_type = DECL_FRAME_TIME;
            } else {
                error("Unknown flip book token declaration '%s'", token);
                continue;
            }

            switch (decl_type) {
            case DECL_TEXTURE: {
                const char *sv = str_token(null, DELIMITERS);

                Assert(meta.count < R_Flip_Book::MAX_FRAMES);
                meta.textures[meta.count] = sid_intern(sv);
                meta.count += 1;
                
                break;
            }
            case DECL_FRAME_TIME: {
                const char *sv = str_token(null, DELIMITERS);
                const f32 v = str_to_f32(sv);
                meta.next_frame_time = v;
                
                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

static void parse_asset_file_data_mesh(const void *data, R_Mesh::Meta &meta,
                                       u16 components_count, u16 *components,
                                       u32 vertices_size, void *vertices,
                                       u32 indices_size, void *indices)  {
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

    meta = {};
        
    const char *p = (const char *)data;

    u8 *vertex_data = (u8 *)vertices;
    u8 *index_data  = (u8 *)indices;
    
    u32 vertex_offsets[R_Vertex_Descriptor::MAX_BINDINGS];

    s32 decl_to_vertex_layout_index[DECL_COUNT];
    u16 decl_to_vct[DECL_COUNT];

    u32 index_data_offset = 0;

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    const char *new_line = str_char(p, ASCII_NEW_LINE);
    
    u16 vertex_layout_size = 0;
    
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
                meta.vertex_count = str_to_u32(sv);
                break;
            }
            case DECL_INDEX_COUNT: {
                char *sv = str_token(null, DELIMITERS);
                meta.index_count = str_to_u32(sv);
                break;
            }
            case DECL_VERTEX_COMPONENT: {
                char *sdecl = str_token(null, DELIMITERS);
                char *stype = str_token(null, DELIMITERS);

                u16 vct;
                if (str_cmp(stype, TYPE_NAME_VEC2)) {
                    vct = R_F32_2;
                } else if (str_cmp(stype, TYPE_NAME_VEC3)) {
                    vct = R_F32_3;
                } else if (str_cmp(stype, TYPE_NAME_VEC4)) {
                    vct = R_F32_4;
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
                decl_to_vertex_layout_index[decl_vertex_type] = vertex_layout_size;

                Assert(vertex_layout_size < components_count);
                components[vertex_layout_size] = vct;
                vertex_layout_size += 1;
                
                break;
            }
            case DECL_DATA_GO: {
                meta.vertex_component_count = vertex_layout_size;
                
                u32 offset = 0;
                for (u32 i = 0; i < vertex_layout_size; ++i) {
                    const u16 vcs = r_vertex_component_size(components[i]); 
                    meta.vertex_size += vcs;
                    
                    vertex_offsets[i] = offset;

                    const u32 size = vcs * meta.vertex_count;
                    offset += size;
                }
                
                const u32 vertex_data_size = meta.vertex_count * meta.vertex_size;
                Assert(vertex_data_size <= vertices_size);
                
                const u32 index_data_size = meta.index_count * sizeof(u32);
                Assert(index_data_size <= indices_size);
                
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
                case R_F32_2: {
                    vec2 v;
                    for (s32 i = 0; i < 2; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    mem_copy(vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case R_F32_3: {
                    vec3 v;
                    for (s32 i = 0; i < 3; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    mem_copy(vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                case R_F32_4: {
                    vec4 v;
                    for (s32 i = 0; i < 4; ++i) {
                        char *sv = str_token(null, DELIMITERS);
                        v[i] = str_to_f32(sv);
                    }

                    u32 offset = vertex_offsets[vli];
                    mem_copy(vertex_data + offset, &v, sizeof(v));
                    vertex_offsets[vli] += sizeof(v);
                    
                    break;
                }
                }
                
                break;
            }
            case DECL_INDEX: {
                char *sv = str_token(null, DELIMITERS);

                const u32 v = str_to_u32(sv);
                mem_copy(index_data + index_data_offset, &v, sizeof(v));
                
                index_data_offset += sizeof(v);
                
                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

#include <sstream>

static void parse_asset_file_data_obj(const void *data, R_Mesh::Meta &meta,
                                      u16 components_count, u16 *components,
                                      u32 vertices_size, void *vertices,
                                      u32 indices_size, void *indices) {
    const auto sdata = std::string((char *)data);
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

                    const vec3 pos = vec3(vx, vy, vz);
                    mem_copy(vertex_data + vertex_offsets[0], &pos, sizeof(pos));
                    vertex_offsets[0] += sizeof(pos);
                }
                
                if (idx.normal_index >= 0) {
                    const f32 nx = attrib.normals[3 * idx.normal_index + 0];
                    const f32 ny = attrib.normals[3 * idx.normal_index + 1];
                    const f32 nz = attrib.normals[3 * idx.normal_index + 2];

                    const vec3 n = vec3(nx, ny, nz);
                    mem_copy(vertex_data + vertex_offsets[1], &n, sizeof(n));
                    vertex_offsets[1] += sizeof(n);
                }

                if (idx.texcoord_index >= 0) {
                    const f32 tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    const f32 ty = attrib.texcoords[2 * idx.texcoord_index + 1];

                    const vec2 uv = vec2(tx, ty);
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
    u32 meta_size = get_asset_meta_size(asset.type);
    void *meta = get_asset_meta(asset.type);
    
    char path[MAX_PATH_SIZE];
    to_full_asset_path(path, sid_str(asset.path));
    
    const u32 max_data_size = get_asset_max_file_size(asset.type);
    void *data = alloct(max_data_size);
    defer { freet(max_data_size); };

    u64 data_size = 0;
    os_read_file(path, data, max_data_size, &data_size);

    ((char *)data)[data_size] = '\0';
    
    switch (asset.type) {
    case ASSET_SHADER: {
        char *src = (char *)data;
            
        // @Todo: this is super risky, make it safe.
        parse_shader_includes(src);
        
        data_size = str_size(src);
        
        break;
    }
    case ASSET_TEXTURE: {
        auto &tmeta = *(R_Texture::Meta *)meta;

        s32 w, h, cc;
        data = stbi_load_from_memory((u8 *)data, (s32)data_size, &w, &h, &cc, 0);

        tmeta.width  = w;
        tmeta.height = h;
        tmeta.format = r_format_from_channel_count(cc);

        // @Cleanup: default values, not really fine.
        tmeta.type = R_TEXTURE_2D;
        tmeta.wrap = R_REPEAT;
        tmeta.min_filter = R_NEAREST_MIPMAP_LINEAR;
        tmeta.mag_filter = R_NEAREST;

        data_size = w * h * cc;
        
        break;
    }
    case ASSET_MATERIAL: {
        auto &mmeta = *(R_Material::Meta *)meta;
        R_Uniform::Meta umetas[R_Material::MAX_UNIFORMS] = { 0 };
        parse_asset_file_data(data, mmeta, COUNT(umetas), umetas);

        data_size = mmeta.uniform_count * sizeof(umetas[0]);
        mem_copy(data, umetas, data_size);
        
        break;
    }
    case ASSET_MESH: {
        constexpr u32 VERTICES_SIZE = MB(16);
        constexpr u32 INDICES_SIZE  = MB(4);
        
        auto &mmeta = *(R_Mesh::Meta *)meta;

        u16 components[R_Vertex_Binding::MAX_COMPONENTS];
        void *vertices = alloct(VERTICES_SIZE);
        void *indices  = alloct(INDICES_SIZE);

        defer { freet(INDICES_SIZE); };
        defer { freet(VERTICES_SIZE); };
        
        const char *extension = str_char_from_end(path, '.');
        if (str_cmp(extension, ".mesh")) {
            parse_asset_file_data_mesh(data, mmeta,
                                       COUNT(components), components,
                                       VERTICES_SIZE, vertices,
                                       INDICES_SIZE, indices);
        } else if (str_cmp(extension, ".obj")) {
            parse_asset_file_data_obj(data, mmeta,
                                      COUNT(components), components,
                                      VERTICES_SIZE, vertices,
                                      INDICES_SIZE, indices);
        }

        const u32 comps_size    = mmeta.vertex_component_count * sizeof(components[0]);
        const u32 vertices_size = mmeta.vertex_count * mmeta.vertex_size;
        const u32 indices_size  = mmeta.index_count  * sizeof(u32);

        u8 *dst = (u8 *)data;
        u32 write_size = 0;
        
        mem_copy(dst + write_size, components, comps_size);    write_size += comps_size;
        mem_copy(dst + write_size, vertices,   vertices_size); write_size += vertices_size;
        mem_copy(dst + write_size, indices,    indices_size);  write_size += indices_size;
        
        data_size = write_size;
        
        break;
    }
    case ASSET_FLIP_BOOK: {
        data_size = 0;

        auto &mmeta = *(R_Flip_Book::Meta *)meta;
        parse_asset_file_data(data, mmeta);

        break;
    }
    case ASSET_SOUND: {
        auto &smeta = *(Au_Sound::Meta *)meta;

        Wav_Header wav;
        data = parse_wav(data, &wav);

        smeta.channel_count = wav.channel_count;
        smeta.sample_rate   = wav.samples_per_second;
        smeta.bit_rate      = wav.bits_per_sample;

        data_size = wav.sampled_data_size;

        break;
    }
    }
    
    asset.index = 0;
    asset.pak_meta_size = meta_size;
    asset.pak_data_size = (u32)data_size;
    
    os_write_file(file, &asset, sizeof(asset));

    os_set_file_ptr(file, asset.pak_blob_offset);
    os_write_file(file, meta, meta_size);
    os_write_file(file, data, data_size);
}

void deserialize(File file, Asset &asset) {
    os_read_file(file, &asset, sizeof(asset));

    os_set_file_ptr(file, asset.pak_blob_offset);

    void *meta = alloct(asset.pak_meta_size);
    void *data = alloct(asset.pak_data_size);

    defer { freet(asset.pak_data_size); };
    defer { freet(asset.pak_meta_size); };

    os_read_file(file, meta, asset.pak_meta_size);
    os_read_file(file, data, asset.pak_data_size);

    // @Safety: a bit unsafe.
    ((char *)data)[asset.pak_data_size] = '\0';

    switch (asset.type) {
    case ASSET_SHADER: {
        char *src = (char *)data;        
        asset.index = r_create_shader(src);
        break;
    }
    case ASSET_TEXTURE: {
        const auto &tmeta = *(R_Texture::Meta *)meta;
        asset.index = r_create_texture(tmeta.type, tmeta.format, tmeta.width, tmeta.height,
                                       tmeta.wrap, tmeta.min_filter, tmeta.mag_filter, data);
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

        void *font_data = allocp(asset.pak_data_size);
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

void save_asset_pack(const char *path) {
    START_SCOPE_TIMER(save);

    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_WRITE);
    defer { os_close_file(file); };
    
    if (file == INVALID_FILE) {
        log("Asset pak %s does not exist, creating new one", path);
        file = os_open_file(path, FILE_OPEN_NEW, FILE_FLAG_WRITE);
        if (file == INVALID_FILE) {
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

    os_write_file(file, &header, sizeof(header));

    For (ast.table) {
        Asset asset;
        asset.type = it.value.asset_type;
        asset.path = it.value.sid_relative_path;
        asset.pak_blob_offset = offset;

        os_set_file_ptr(file, offsets[asset.type]);
        serialize(file, asset);

        offsets[asset.type] += sizeof(Asset);
        offset += get_asset_blob_size(asset);
    }
    
    log("Saved asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(save));
}

void load_asset_pack(const char *path) {
    START_SCOPE_TIMER(load);

    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
    defer { os_close_file(file); };
    
    if (file == INVALID_FILE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    Asset_Pak_Header header;
    os_read_file(file, &header, sizeof(header));

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

            add(Asset_table, asset.path, asset);
        }
    }
    
    log("Loaded asset pack %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void to_relative_asset_path(char *relative_path, const char *full_path) {
    const char *data = str_sub(full_path, "\\data");
    if (!data) data = str_sub(full_path, "/data");
    if (!data) return;

    str_copy(relative_path, data);
    fix_directory_delimiters(relative_path);
}

void to_full_asset_path(char *full_path, const char *relative_path) {
    str_copy(full_path, DIR_RUN_TREE);
    str_glue(full_path, relative_path);
    fix_directory_delimiters(full_path);
}
