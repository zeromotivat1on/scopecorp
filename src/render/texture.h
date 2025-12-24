#pragma once

#include "hash_table.h"
#include "catalog.h"
#include "gpu.h"

inline constexpr auto PNG_EXT  = S("png");
inline constexpr auto JPG_EXT  = S("jpg");
inline constexpr auto JPEG_EXT = S("jpeg");

inline constexpr String TEXTURE_EXTS[] = { PNG_EXT, JPG_EXT, JPEG_EXT };

enum Texture_Type : u8 {
    TEX_TYPE_NONE,
    TEX_2D,
    TEX_2D_ARRAY,
};

enum Texture_Format_Type : u8 {
    TEX_FORMAT_NONE,
    TEX_RED_8,
    TEX_RED_32,
    TEX_RED_32U,
    TEX_RGB_8,
    TEX_RGBA_8,
    TEX_DEPTH_24_STENCIL_8,
};

enum Texture_Wrap_Type : u8 {
    TEX_WRAP_NONE,
    TEX_REPEAT,
    TEX_CLAMP,
    TEX_BORDER,
};

enum Texture_Filter_Type : u8 {
    TEX_FILTER_NONE,
    TEX_LINEAR,
    TEX_NEAREST,
    TEX_LINEAR_MIPMAP_LINEAR,
    TEX_LINEAR_MIPMAP_NEAREST,
    TEX_NEAREST_MIPMAP_LINEAR,
    TEX_NEAREST_MIPMAP_NEAREST,
};

struct Loaded_Texture {
    String path;
    Buffer contents;
    
    u16 width  = 0;
    u16 height = 0;
    
    Texture_Type        type;
    Texture_Format_Type format;
};

struct Texture {
    String path;
    String name;
    
    Gpu_Handle handle = GPU_HANDLE_NONE;
    
    u16 width  = 0;
    u16 height = 0;
    
    Texture_Type        type;
    Texture_Format_Type format;
    Texture_Wrap_Type   wrap;
    Texture_Filter_Type min_filter;
    Texture_Filter_Type mag_filter;
};

struct Global_Textures {
    Texture *error = null;
};

inline Catalog texture_catalog;
inline Table <String, Texture> texture_table;
inline Global_Textures global_textures;

void preload_all_textures ();

Texture       *new_texture  (String path);
Texture       *new_texture  (String path, Buffer contents);
Texture       *new_texture  (const Loaded_Texture &loaded_texture);
Texture       *get_texture  (String name);
Loaded_Texture load_texture (String path);
Loaded_Texture load_texture (String path, Buffer contents);

void update_texture_filters (Texture &texture, Texture_Filter_Type min_filter, Texture_Filter_Type mag_filter);

bool is_valid (const Loaded_Texture &loaded_texture);
bool is_valid (const Texture &texture);

bool is_texture_path (String path);
