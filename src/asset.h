#pragma once

enum Asset_Type : u8 {
    ASSET_TYPE_NONE,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_FLIP_BOOK,
    ASSET_TYPE_MESH,
    ASSET_TYPE_FONT,
    ASSET_TYPE_SOUND,
};

inline constexpr auto OBJ_INDEX_LISTS_PER_FACE = 4;

struct Obj_Face {
    u32 position_indices[OBJ_INDEX_LISTS_PER_FACE];
    u32 texcoord_indices[OBJ_INDEX_LISTS_PER_FACE];
    u32 normal_indices  [OBJ_INDEX_LISTS_PER_FACE];
};

struct Parsed_Obj {
    Array <Vector4>  positions;
    Array <Vector3>  texcoords;
    Array <Vector3>  normals;
    Array <Obj_Face> faces;
};

inline const auto LOG_IDENT_OBJ = S("obj");

// Parse object file from it's given contents, path is used for debug logging.
Parsed_Obj parse_obj_file (String path, String contents, Allocator alc);

inline u32 get_obj_face_index_count(const Obj_Face &face) {
    auto n = 0u;
    while (face.position_indices[n]) n += 1;
    Assert(n <= OBJ_INDEX_LISTS_PER_FACE);
    return n;
};
