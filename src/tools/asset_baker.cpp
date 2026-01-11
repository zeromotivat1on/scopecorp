#define SPRINTF_CUSTOM_STRING
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "basic.cpp"
#include "os.cpp"

#ifdef _WIN32
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dbghelp.lib")
#include "win32.cpp"
#endif

#include "font.h"
#include "profile.h"
#include "catalog.h"
#include "game_pak.h"

#include "stb_truetype.h"
#include "stb_image_write.h"

struct Asset_Set_Description {
    Asset_Type asset_type;
    String path;
    Catalog catalog;
};

struct Font_Bake_Data {
    s32 pixel_height;
    s32 width;
    s32 height;
};

s32 main() {
    
    set_process_cwd(get_process_directory());

    Asset_Set_Description sets[] = {
        { ASSET_SHADER,    PATH_SHADER(""),    },
        { ASSET_FONT,      PATH_FONT(""),      },
        { ASSET_TEXTURE,   PATH_TEXTURE(""),   },
        { ASSET_MATERIAL,  PATH_MATERIAL(""),  },
        { ASSET_SOUND,     PATH_SOUND(""),     },
        { ASSET_FLIP_BOOK, PATH_FLIP_BOOK(""), },
        { ASSET_MESH,      PATH_MESH(""),      },
    };

    START_TIMER(0);

    Array <String> generated_assets;
    generated_assets.allocator = __temporary_allocator;
    array_realloc(generated_assets, 16);

    static const auto generate_h = [&](String path, String source) {
        write_text_file(path, source);
        array_add(generated_assets, make_string(path.data + 3, path.size - 3));
    };
    
    // Generate default missing fallback assets.
    for (u32 i = 0; i < carray_count(sets); ++i) {
        const auto &set = sets[i];
        switch (set.asset_type) {
        case ASSET_TEXTURE: {
            constexpr auto w    = 64;
            constexpr auto h    = 64;
            constexpr auto cc   = 4;
            constexpr auto grid = 8;

            u32 pixels[w * h];
            for (s32 y = 0; y < h; ++y) {                
                for (s32 x = 0; x < w; ++x) {
                    const auto i = x + y * w;
                    if ((x / grid + y / grid) % 2 == 0) {
                        pixels[i] = 0xFF000000;
                    } else {
                        pixels[i] = 0xFFFF00FF;
                    }
                }
            }
            
            String_Builder sb;
            codegen(sb, "#pragma once");
            codegen(sb, "static const u32 missing_texture_width  = %d;", w);
            codegen(sb, "static const u32 missing_texture_height = %d;", h);
            codegen(sb, "static const u32 missing_texture_color_channel_count = %d;", cc);
            codegen(sb, "static const u32 missing_texture_pixels[] = {");
            for (s32 i = 0; i < w * h; i += 8) {
                codegen(sb, "    0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X,",
                        pixels[i + 0], pixels[i + 1], pixels[i + 2], pixels[i + 3],
                        pixels[i + 4], pixels[i + 5], pixels[i + 6], pixels[i + 7]);
            }
            codegen(sb, "};");

            generate_h(PATH_CODEGEN("missing_texture.h"), builder_to_string(sb));
            break;
        }
        case ASSET_SHADER: {
            static const char *shader_source[] = {
                ""
            };
            
            String_Builder sb;
            codegen(sb, "#pragma once");
            codegen(sb, "static const char *missing_shader_source = ");
            for (u32 i = 0; i < carray_count(shader_source); ++i) {
                codegen(sb, "\"%s\\n\"", shader_source[i]);
            }
            codegen(sb, ";");
            
            generate_h(PATH_CODEGEN("missing_shader.h"), builder_to_string(sb));
            
            break;
        };
        case ASSET_MATERIAL: {
            static const char *material_source[] = {
                "s  missing",
                "ta missing",
                "td missing",
                "ts missing",
            };
            
            String_Builder sb;
            codegen(sb, "#pragma once");
            codegen(sb, "static const char *missing_material_source = ");
            for (u32 i = 0; i < carray_count(material_source); ++i) {
                codegen(sb, "\"%s\\n\"", material_source[i]);
            }
            codegen(sb, ";");
            
            generate_h(PATH_CODEGEN("missing_material.h"), builder_to_string(sb));
            break;
        }
        case ASSET_MESH: {
            static const char *cube_obj[] = {
                "o cube",
                "v -0.5 -0.5 -0.5",
                "v  0.5 -0.5 -0.5",
                "v  0.5  0.5 -0.5",
                "v -0.5  0.5 -0.5",
                "v -0.5 -0.5  0.5",
                "v  0.5 -0.5  0.5",
                "v  0.5  0.5  0.5",
                "v -0.5  0.5  0.5",
                "vn  0  0 -1",
                "vn  0  0  1",
                "vn -1  0  0",
                "vn  1  0  0",
                "vn  0 -1  0",
                "vn  0  1  0",
                "vt 0 0",
                "vt 1 0",
                "vt 1 1",
                "vt 0 1",
                "f 1/1/1 2/2/1 3/3/1",
                "f 1/1/1 3/3/1 4/4/1",
                "f 5/1/2 7/3/2 6/2/2",
                "f 5/1/2 8/4/2 7/3/2",
                "f 1/1/3 4/4/3 8/3/3",
                "f 1/1/3 8/3/3 5/2/3",
                "f 2/1/4 6/2/4 7/3/4",
                "f 2/1/4 7/3/4 3/4/4",
                "f 1/1/5 5/2/5 6/3/5",
                "f 1/1/5 6/3/5 2/4/5",
                "f 4/1/6 3/4/6 7/3/6",
                "f 4/1/6 7/3/6 8/2/6",
            };

            String_Builder sb;
            codegen(sb, "#pragma once");
            codegen(sb, "static const char *missing_mesh_obj = ");
            for (u32 i = 0; i < carray_count(cube_obj); ++i) {
                codegen(sb, "\"%s\\n\"", cube_obj[i]);
            }
            codegen(sb, ";");

            generate_h(PATH_CODEGEN("missing_mesh.h"), builder_to_string(sb));
            break;
        }
        }
    }

    const auto generate_ms = CHECK_TIMER_MS(0);

    For (generated_assets) print("%S, ", it);
    print("\n\n");
    
    START_TIMER(1);
        
    for (u32 i = 0; i < carray_count(sets); ++i) {
        auto &set = sets[i];
        add_directory_files(&set.catalog, set.path);
    }
    
    Create_Pak pak;

    Array <String> baked_assets;
    baked_assets.allocator = __temporary_allocator;
    array_realloc(baked_assets, 512);

    const auto save_to_pak = [&](String path, Buffer buffer, Asset_Type asset_type) {
        add(pak, path, buffer, asset_type);
        array_add(baked_assets, path);
    };
    
    // Bake assets.
    for (u32 i = 0; i < carray_count(sets); ++i) {
        const auto &set = sets[i];

        switch (set.asset_type) {
            // Font bake produces atlas images, so it should run before texture bake.
        case ASSET_FONT: {
            static u8              pixels[1 << 22];
            static stbtt_bakedchar cdata [96]; // ASCII 32..126 is 95 glyphs

            static const Font_Bake_Data font_bake_data[] = {
                { 16, 128, 128 },
                { 24, 256, 256 },
            };
            
            For (set.catalog.entries) {
                const auto buffer = read_file(it.path, __temporary_allocator);
                const auto first_char = 32;
                const auto char_count = carray_count(cdata);

                stbtt_fontinfo font;
                if (!stbtt_InitFont(&font, buffer.data, 0)) {
                    print("[stbtt] Failed to init %S\n", it.path);
                    continue;
                }

                s32 ascent, descent, line_gap;
                stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

                for (u32 j = 0; j < carray_count(font_bake_data); ++j) {
                    const auto &bake_data   = font_bake_data[j];
                    const auto pixel_height = bake_data.pixel_height;
                    const auto width        = bake_data.width;
                    const auto height       = bake_data.height;

                    const auto atlas_dir  = DIR_TEXTURES;
                    const auto atlas_name = get_file_name_no_ext(it.path);
                    const auto atlas_ext  = S(".png");
                    const auto atlas_path = tprint("%S%S_%d%S", atlas_dir, atlas_name, pixel_height, atlas_ext);
                    
                    const auto stbtt_n = stbtt_BakeFontBitmap(buffer.data, 0, (float)pixel_height,
                                                              pixels, width, height,
                                                              first_char, char_count, cdata);
                    if (stbtt_n == 0) {
                        print("[stbtt] Failed to bake %S\n", it.path);
                        continue;
                    }

                    const auto atlas_cpath = temp_c_string(atlas_path);
                    if (!stbi_write_png(atlas_cpath, width, abs(stbtt_n), 1, pixels, width * sizeof(pixels[0]))) {
                        print("[stbi] Failed to bake %S\n", atlas_path);
                        continue;
                    }

                    // Find texture catalog and add baked font atlas image there.
                    for (s32 k = i; k < carray_count(sets); ++k) {
                        auto &set = sets[k];
                        if (set.asset_type == ASSET_TEXTURE) {
                            Catalog_Entry entry;
                            entry.path = atlas_path;
                            entry.name = atlas_name;
                            // @Todo: fix entry add if baked atlases already exist on disk.
                            add_entry(&set.catalog, entry);
                        }
                    }

                    const auto px_h_scale  = stbtt_ScaleForPixelHeight(&font, (float)pixel_height);
                    const auto line_height = (s32)((ascent - descent + line_gap) * px_h_scale);

                    Baked_Font_Meta meta;
                    meta.atlas_path_size = (u32)atlas_path.size;
                    meta.start_charcode  = first_char;
                    meta.charcode_count  = char_count;
                    meta.ascent          = ascent;
                    meta.descent         = descent;
                    meta.line_gap        = line_gap;
                    meta.px_h_scale      = px_h_scale;
                    meta.px_height       = pixel_height;
                    meta.line_height     = line_height;

                    // Ensure correct glyphs.
                    auto glyphs = New(Glyph, meta.charcode_count, __temporary_allocator);
                    for (u32 i = 0; i < meta.charcode_count; ++i) {
                        glyphs[i].x0       = cdata[i].x0;
                        glyphs[i].y0       = cdata[i].y0;
                        glyphs[i].x1       = cdata[i].x1;
                        glyphs[i].y1       = cdata[i].y1;
                        glyphs[i].xoff     = cdata[i].xoff;
                        glyphs[i].yoff     = cdata[i].yoff;
                        glyphs[i].xadvance = cdata[i].xadvance;
                    }

                    String_Builder sb;
                    sb.allocator = __temporary_allocator;

                    put   (sb, meta);
                    append(sb, atlas_path.data, atlas_path.size);                    
                    append(sb, glyphs, meta.charcode_count * sizeof(glyphs[0]));

                    const auto meta_dir  = DIR_FONTS;
                    const auto meta_name = get_file_name_no_ext(it.path);
                    const auto meta_ext  = get_extension(it.path);
                    const auto meta_path = tprint("%S%S_%d.%S", meta_dir, meta_name, pixel_height, meta_ext);
                    
                    const auto baked_font_meta = make_buffer(builder_to_string(sb));
                    save_to_pak(meta_path, baked_font_meta, ASSET_FONT);
                }
            }
            
            break;
        }
        default: {
            For (set.catalog.entries) {
                const auto buffer = read_file(it.path, __temporary_allocator);
                save_to_pak(it.path, buffer, set.asset_type);
            }
            break;
        }
        }

    }

    write(pak, GAME_PAK_PATH);

    const auto bake_ms = CHECK_TIMER_MS(1);
    
    For (baked_assets) print("%S, ", it);
    print("\n\n");

    print("Generated missing assets %.2fms\n", generate_ms);
    print("Baked %S %.2fms\n", GAME_PAK_PATH, bake_ms);

    return 0;
}
