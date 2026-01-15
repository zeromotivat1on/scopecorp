#pragma once

inline const auto MATERIAL_EXT = S("material");

struct Shader;
struct Texture;

struct Material {
    String  shader;
    Vector4 base_color = Vector4_white;
    Vector3 ambient    = Vector3_white;
    Vector3 diffuse    = Vector3_white;
    Vector3 specular   = Vector3_black;
    f32     shininess  = 1.0f;
    Atom    ambient_texture;
    Atom    diffuse_texture;
    Atom    specular_texture;
    bool    use_blending;
    Table <String, Constant_Buffer_Instance> cbi_table;
};

struct Global_Materials {
    Material *missing = null;
};

inline Table <Atom, Material> material_table;
inline Global_Materials       global_materials;

Material *new_material     (String path);
Material *new_material     (Atom name, String contents);
Material *get_material     (Atom name);
bool      has_transparency (const Material *material);
