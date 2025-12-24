#pragma once

#include "catalog.h"

inline constexpr auto MATERIAL_EXT = S("material");

struct Shader;
struct Texture;

struct Material {
    String name;
    String path;
    
    Shader *shader = null;

    Vector4 base_color = Vector4_white;
    
    Vector3 ambient   = Vector3_white;
    Vector3 diffuse   = Vector3_white;
    Vector3 specular  = Vector3_black;
    f32  shininess = 1.0f;

    Texture *ambient_texture  = null;
    Texture *diffuse_texture  = null;
    Texture *specular_texture = null;

    Table <String, Constant_Buffer_Instance> cbi_table;

    bool use_blending = false;
};

inline Catalog material_catalog;
inline Table <String, Material> material_table;

Material *new_material (String path);
Material *new_material (String path, String contents);
Material *new_material (String name, Shader *shader);
Material *get_material (String name);

bool has_transparency (const Material *material);
