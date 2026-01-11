#pragma once

#include "hash_table.h"
#include "catalog.h"

inline const auto PNG_EXT  = S("png");
inline const auto JPG_EXT  = S("jpg");
inline const auto JPEG_EXT = S("jpeg");

inline const String TEXTURE_EXTS[] = { PNG_EXT, JPG_EXT, JPEG_EXT };

struct Texture {
    u32 image_view;
    u32 sampler;
};

struct Global_Textures {
    Texture *missing = null;
};

inline Table <Atom, Texture> texture_table;
inline Global_Textures       global_textures;

Texture *new_texture (String path);
Texture *new_texture (Atom name, Buffer file_contents);
Texture *new_texture (Atom name, u32 image_view, u32 sampler);
Texture *get_texture (Atom name);

bool is_texture_path (String path);
