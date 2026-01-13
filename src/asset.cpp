#include "pch.h"
#include "asset.h"
#include "text_file.h"
#include "wav.h"
#include "font.h"
#include "shader.h"
#include "texture.h"
#include "material.h"
#include "flip_book.h"
#include "triangle_mesh.h"

Parsed_Obj parse_obj_file(String path, String contents, Allocator alc) {
    static const auto v_token       = S("v");
    static const auto f_token       = S("f");
    static const auto l_token       = S("l");
    static const auto o_token       = S("o");
    static const auto g_token       = S("g");
    static const auto s_token       = S("s");
    static const auto vt_token      = S("vt");
    static const auto vn_token      = S("vn");
    static const auto vp_token      = S("vp");
    static const auto mtllib_token  = S("mtllib");
    static const auto usemtl_token  = S("usemtl");
    static const auto comment_char  = '#';
    
    Text_File_Handler handler;
    handler.allocator = alc;
    defer { reset(&handler); };
    
    if (!init_from_memory(&handler, contents)) {
        log(LOG_ERROR, "Failed to load %S", path);
        return {};
    }

    const auto file_size             = (u32)handler.contents.size;
    const auto approx_line_count     = file_size / 32;
    const auto approx_face_count     = approx_line_count / 4;
    const auto approx_position_count = approx_face_count;
    const auto approx_texcoord_count = approx_face_count;
    const auto approx_normal_count   = approx_face_count;

    Parsed_Obj obj;
    obj.positions.allocator = alc;
    obj.texcoords.allocator = alc;
    obj.normals  .allocator = alc;
    obj.faces    .allocator = alc;
    array_realloc(obj.positions, approx_position_count);
    array_realloc(obj.texcoords, approx_texcoord_count);
    array_realloc(obj.normals,   approx_normal_count);
    array_realloc(obj.faces,     approx_face_count);
    array_add    (obj.positions, {});
    array_add    (obj.texcoords, {});
    array_add    (obj.normals,   {});

    u32 line_index = 0;
    while (1) {
        defer { line_index += 1; };
        
        const auto line = read_line(&handler);
        if (!line) break;

        const auto line_delims = S(" ");
        const auto line_tokens = split(line, line_delims, alc);
        if (!line_tokens.count) continue;
        
        const auto obj_token = line_tokens[0];
        if (obj_token == v_token) {
            auto position = Vector4(0, 0, 0, 1);
            for (u32 i = 1; i < line_tokens.count; ++i) {
                const auto token = line_tokens[i];
                position[i - 1] = (f32)string_to_float(token);
            }

            array_add(obj.positions, position);
        } else if (obj_token == f_token) {
            auto face = Obj_Face {};

            auto pv  = face.position_indices;
            auto pvt = face.texcoord_indices;
            auto pvn = face.normal_indices;

            auto list_count = line_tokens.count - 1;
            if (list_count > OBJ_INDEX_LISTS_PER_FACE) {
                log(LOG_IDENT_OBJ, LOG_ERROR, "Unsupported polygon element count %u (max %u), clamping to max", line_tokens.count - 1, OBJ_INDEX_LISTS_PER_FACE);
                list_count = OBJ_INDEX_LISTS_PER_FACE;
            }
            
            for (u32 i = 0; i < list_count; ++i) {
                const auto list_token   = line_tokens[i + 1]; // ignore first line token
                const auto index_delims = S("/");
                const auto index_tokens = split(list_token, index_delims);
                
                switch (index_tokens.count) {
                case 1: {
                    *pv = (u32)string_to_integer(index_tokens[0]); pv += 1;
                    break;
                }
                case 2: {
                    *pv = (u32)string_to_integer(index_tokens[0]); pv += 1;

                    const auto next_is_texcoord = find(list_token, S("//")) == INDEX_NONE;
                    if (next_is_texcoord) {
                        *pvt = (u32)string_to_integer(index_tokens[1]); pvt += 1;
                    } else {
                        *pvn = (u32)string_to_integer(index_tokens[1]); pvn += 1;
                    }
                    
                    break;
                }
                case 3: {
                    *pv  = (u32)string_to_integer(index_tokens[0]); pv  += 1;
                    *pvt = (u32)string_to_integer(index_tokens[1]); pvt += 1;
                    *pvn = (u32)string_to_integer(index_tokens[2]); pvn += 1;
                    break;
                }
                default: {
                    log(LOG_IDENT_OBJ, LOG_ERROR, "Invalid obj face %u vertex list %u count %u in %S", obj.faces.count - 1, i - 1, index_tokens.count, path);
                    break;
                }
                }

                Assert(pv  - face.position_indices <= OBJ_INDEX_LISTS_PER_FACE);
                Assert(pvt - face.texcoord_indices <= OBJ_INDEX_LISTS_PER_FACE);
                Assert(pvn - face.normal_indices   <= OBJ_INDEX_LISTS_PER_FACE);
            }
            
            if (list_count <= 3) {
                array_add(obj.faces, face);
            } else if (list_count == 4) {
                // Triangulate using a triangle fan.
                Obj_Face faces[2];
                faces[0].position_indices[0] = face.position_indices[0];
                faces[0].position_indices[1] = face.position_indices[1];
                faces[0].position_indices[2] = face.position_indices[2];
                faces[0].position_indices[3] = 0;
                faces[0].texcoord_indices[0] = face.texcoord_indices[0];
                faces[0].texcoord_indices[1] = face.texcoord_indices[1];
                faces[0].texcoord_indices[2] = face.texcoord_indices[2];
                faces[0].texcoord_indices[3] = 0;
                faces[0].normal_indices  [0] = face.normal_indices  [0];
                faces[0].normal_indices  [1] = face.normal_indices  [1];
                faces[0].normal_indices  [2] = face.normal_indices  [2];
                faces[0].normal_indices  [3] = 0;
                faces[1].position_indices[0] = face.position_indices[0];
                faces[1].position_indices[1] = face.position_indices[2];
                faces[1].position_indices[2] = face.position_indices[3];
                faces[1].position_indices[3] = 0;
                faces[1].texcoord_indices[0] = face.texcoord_indices[0];
                faces[1].texcoord_indices[1] = face.texcoord_indices[2];
                faces[1].texcoord_indices[2] = face.texcoord_indices[3];
                faces[1].texcoord_indices[3] = 0;
                faces[1].normal_indices  [0] = face.normal_indices  [0];
                faces[1].normal_indices  [1] = face.normal_indices  [2];
                faces[1].normal_indices  [2] = face.normal_indices  [3];
                faces[1].normal_indices  [3] = 0;

                array_add(obj.faces, faces[0]);
                array_add(obj.faces, faces[1]);
            } else {
                log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj face %u list count %u in %S", obj.faces.count, list_count, path);
            }
        } else if (obj_token == l_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == o_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == g_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == s_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == vt_token) {
            auto texcoord = Vector3(0, 0, 0);
            for (u32 i = 1; i < line_tokens.count; ++i) {
                const auto token = line_tokens[i];
                texcoord[i - 1] = (f32)string_to_float(token);
            }

            array_add(obj.texcoords, texcoord);
        } else if (obj_token == vn_token) {
            auto normal = Vector3(0, 0, 0);
            for (u32 i = 1; i < line_tokens.count; ++i) {
                const auto token = line_tokens[i];
                normal[i - 1] = (f32)string_to_float(token);
            }

            array_add(obj.normals, normal);
        } else if (obj_token == vp_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == mtllib_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token == usemtl_token) {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unhandled obj token %S in %S", obj_token, path);
        } else if (obj_token.data[0] == comment_char) {
            // skip comment
        } else {
            log(LOG_IDENT_OBJ, LOG_ERROR, "Unknown obj token %S in %S", obj_token, path);
        }
    }

    return obj;
}
