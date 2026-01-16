#include "pch.h"
#include "game.h"
#include "entity_manager.h"
#include "level.h"
#include "text_file.h"
#include "profile.h"
#include "input.h"
#include "asset.h"
#include "file_system.h"
#include "window.h"
#include "editor.h"
#include "console.h"
#include "render.h"
#include "shader_binding_model.h"
#include "ui.h"
#include "viewport.h"
#include "material.h"
#include "texture.h"
#include "flip_book.h"
#include "audio_player.h"

void game_logger_proc(String message, String ident, Log_Level level, void *logger_data) {
    thread_local char buffer[4096];

    Assert(logger_data);
    auto logger = (Game_Logger_Data *)logger_data;

    String s;
    s.data = (u8 *)buffer;
    if (ident) {
        s.size = stbsp_snprintf(buffer, carray_count(buffer), "[%S] %S\n", ident, message);
    } else {
        s.size = stbsp_snprintf(buffer, carray_count(buffer), "%S\n", message);
    }

    array_add(logger->messages, copy_string(s, logger->allocator));
    add_to_console_history(level, s);
}

void flush_game_logger() {
    For (game_logger_data.messages) print(it);
    array_clear(game_logger_data.messages);
}

void on_window_resize(u32 width, u32 height) {
    resize(screen_viewport, width, height);

    auto manager = get_entity_manager();
    on_viewport_resize(manager->camera, screen_viewport);
}

void init_asset_storages() {
    table_realloc(texture_table,    64);
    table_realloc(material_table,   64);
    table_realloc(mesh_table,       64);
    table_realloc(flip_book_table,  64);
    table_realloc(font_atlas_table, 16);
}

static bool load_game_pak(String path) {
    START_TIMER(0);

    Load_Pak pak;
    
    if (!load(pak, path)) {
        log(LOG_ERROR, "Failed to load %S", path);
        return false;
    }

    For (pak.entries) {
        START_TIMER(0);
        
        const auto &entry = it;
        const auto name = get_file_name_no_ext(it.name);
        const auto atom = make_atom(name);
        
        // @Cleanup: ideally make sort of lookup table for create functions to call,
        // so you can use it something like lut[type](...) instead of switch.
        switch (entry.user_value) {
        case ASSET_TYPE_SHADER:    new_shader    (entry.name, make_string(entry.buffer)); break;
        case ASSET_TYPE_TEXTURE:   new_texture   (atom, entry.buffer); break;
        case ASSET_TYPE_MATERIAL:  new_material  (atom, make_string(entry.buffer)); break;
        case ASSET_TYPE_SOUND:     new_sound     (atom, entry.buffer); break;
        case ASSET_TYPE_FLIP_BOOK: new_flip_book (atom, make_string(entry.buffer)); break;
            // @Todo: determine mesh file format instead of hardcode.
        case ASSET_TYPE_MESH:      new_mesh      (atom, entry.buffer, MESH_FILE_FORMAT_OBJ); break;
        case ASSET_TYPE_FONT:      new_font_atlas(entry.name, entry.buffer); break;
        }

        log(LOG_VERBOSE, "Loaded pak entry %S %.2fms", entry.name, CHECK_TIMER_MS(0));
    }
    
    log("Loaded %S %.2fms", path, CHECK_TIMER_MS(0));
    return true;
}

void load_game_assets() {
    {
        #include "missing_texture.h"

        const auto type   = GPU_IMAGE_TYPE_2D;
        const auto format = gpu_image_format_from_channel_count(missing_texture_color_channel_count);
        const auto mipmap_count = gpu_max_mipmap_count(missing_texture_width, missing_texture_height);
        const auto buffer = make_buffer((void *)missing_texture_pixels, sizeof(missing_texture_pixels));
        
        const auto image = gpu_new_image(type, format, mipmap_count, missing_texture_width, missing_texture_height, 1, buffer);
        const auto image_view = gpu_new_image_view(image, type, format, 0, mipmap_count, 0, 1);
        global_textures.missing = new_texture(ATOM("missing"), image_view, gpu.sampler_default_color);
    }

    {
        #include "missing_shader.h"

        global_shaders.missing = new_shader(S("missing.sl"), make_string((u8 *)missing_shader_source));
    }
       
    {
        #include "missing_material.h"

        global_materials.missing = new_material(ATOM("missing"), make_string((u8 *)missing_material_source));
    }
     
    {
        #include "missing_mesh.h"

        const auto buffer = make_buffer((void *)missing_mesh_obj, cstring_count(missing_mesh_obj));
        global_meshes.missing = new_mesh(ATOM("missing"), buffer, MESH_FILE_FORMAT_OBJ);
    }
     
    load_game_pak(GAME_PAK_PATH);
    
    {
        auto &atlas = global_font_atlases.main_small;
        atlas = get_font_atlas(S("better_vcr_16"));
        atlas->texture = get_texture(make_atom(atlas->texture_name));
    }

    {
        auto &atlas = global_font_atlases.main_medium;
        atlas = get_font_atlas(S("better_vcr_24"));
        atlas->texture = get_texture(make_atom(atlas->texture_name));
    }
    
    // Shader constants.
    {
        auto cb_global_parameters      = get_constant_buffer(S("Global_Parameters"));
        auto cb_level_parameters       = get_constant_buffer(S("Level_Parameters"));
        auto cb_frame_buffer_constants = get_constant_buffer(S("Frame_Buffer_Constants"));

        cbi_global_parameters      = make_constant_buffer_instance(cb_global_parameters);
        cbi_level_parameters       = make_constant_buffer_instance(cb_level_parameters);
        cbi_frame_buffer_constants = make_constant_buffer_instance(cb_frame_buffer_constants);
        
        {
            auto &table = cbi_global_parameters.value_table;
            cv_viewport_cursor_pos = table_find(table, S("viewport_cursor_pos"));
            cv_viewport_resolution = table_find(table, S("viewport_resolution"));
            cv_viewport_ortho      = table_find(table, S("viewport_ortho"));
            cv_camera_position     = table_find(table, S("camera_position"));
            cv_camera_view         = table_find(table, S("camera_view"));
            cv_camera_proj         = table_find(table, S("camera_proj"));
            cv_camera_view_proj    = table_find(table, S("camera_view_proj"));
        }

        {
            auto &table = cbi_level_parameters.value_table;
            cv_direct_light_count = table_find(table, S("direct_light_count"));
            cv_point_light_count  = table_find(table, S("point_light_count"));
            cv_direct_lights      = table_find(table, S("direct_lights"));
            cv_point_lights       = table_find(table, S("point_lights"));
        }

        {
            auto &table = cbi_frame_buffer_constants.value_table;
            cv_fb_transform                = table_find(table, S("transform"));
            cv_resolution                  = table_find(table, S("resolution"));
            cv_pixel_size                  = table_find(table, S("pixel_size"));
            cv_curve_distortion_factor     = table_find(table, S("curve_distortion_factor"));
            cv_chromatic_aberration_offset = table_find(table, S("chromatic_aberration_offset"));
            cv_quantize_color_count        = table_find(table, S("quantize_color_count"));
            cv_noise_blend_factor          = table_find(table, S("noise_blend_factor"));
            cv_scanline_count              = table_find(table, S("scanline_count"));
            cv_scanline_intensity          = table_find(table, S("scanline_intensity"));
        }
    }
}

void on_push_base(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_BASE);
}

void on_pop_base(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_BASE);    
}

bool on_event_base(Program_Layer *layer, const Window_Event *e) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_BASE);

    auto input = get_input_table();
    // Update input table and handle base common events like window resize/close/etc.
    switch (e->type) {
    case WINDOW_EVENT_RESIZE: {
        on_window_resize(e->w, e->h);
        break;
    }
    case WINDOW_EVENT_QUIT: {
        should_quit_game = true;
        break;
    }
    case WINDOW_EVENT_FOCUSED: {
        break;
    }
    case WINDOW_EVENT_LOST_FOCUS: {
        clear_bit(&input->keys, KEY_ALT);
        break;
    }
    case WINDOW_EVENT_KEYBOARD:
    case WINDOW_EVENT_MOUSE_BUTTON: {
        const auto key = e->key_code;
        auto keys = &input->keys;

        if (e->press || e->repeat) set_bit(keys, key);
        else clear_bit(keys, key);
        
        break;
    }
    case WINDOW_EVENT_MOUSE_MOVE: {
        input->cursor_offset_x =  e->x;
        input->cursor_offset_y = -e->y;

        auto window = get_window();
        if (window->cursor_locked) {
            const auto x = input->cursor_x = window->width  / 2;
            const auto y = input->cursor_y = window->height / 2;
            set_cursor(window, x, y);
        } else {
            get_cursor(window, &input->cursor_x, &input->cursor_y);
        }
            
        break;
    }
    }

    auto handled = on_event_input_mapping(layer, e);
    if (handled) return handled;
    
    return handled;
}

bool on_event_input_mapping(Program_Layer *layer, const Window_Event *e) {
    Assert(layer->type != PROGRAM_LAYER_TYPE_NONE);

    auto handled = true;
    switch (e->type) {
    case WINDOW_EVENT_KEYBOARD:
    case WINDOW_EVENT_MOUSE_BUTTON: {
        // Ignore mouse clicks on title bar.
        const auto input = get_input_table();
        if (e->type == WINDOW_EVENT_MOUSE_BUTTON && input->cursor_y < 0) break;

        const auto mapping = table_find(layer->input_mapping_table, e->key_code);
        if (mapping) {
            if      (e->repeat && mapping->on_repeat) mapping->on_repeat();
            else if (e->press  && mapping->on_press)  mapping->on_press();
            else if (mapping->on_release)             mapping->on_release();
        }
            
        break;
    }
    default: { handled = false; break; }
    }

    return handled;
}

void init_program_layers() {
    {
        auto &layer = program_layer_base;
        layer.type = PROGRAM_LAYER_TYPE_BASE;
        layer.on_push  = on_push_base;
        layer.on_pop   = on_pop_base;
        layer.on_event = on_event_base;

        Input_Mapping im;

        im = {};
        im.on_press = []() { close(get_window()); };
        table_add(layer.input_mapping_table, KEY_CLOSE_WINDOW, im);

        im = {};
        im.on_press = []() { auto w = get_window(); lock_cursor(w, !w->cursor_locked); };
        table_add(layer.input_mapping_table, KEY_TOGGLE_CURSOR, im);

        im = {};
        im.on_press = []() {
            const auto layer = get_program_layer();
            if (layer->type == PROGRAM_LAYER_TYPE_CONSOLE) return;

            if (layer->type == PROGRAM_LAYER_TYPE_PROFILER) {
                pop_program_layer();
            }

            push_program_layer(&program_layer_console); 
        };
        table_add(layer.input_mapping_table, KEY_OPEN_CONSOLE, im);

        im = {};
        im.on_press = []() {
            const auto layer = get_program_layer();
            if (layer->type == PROGRAM_LAYER_TYPE_PROFILER) return;

            if (layer->type == PROGRAM_LAYER_TYPE_CONSOLE) {
                pop_program_layer();
            }

            push_program_layer(&program_layer_profiler); 
        };
        table_add(layer.input_mapping_table, KEY_OPEN_PROFILER, im);

        im = {};
        im.on_press = []() {
            if (game_state.polygon_mode == GPU_POLYGON_FILL) {
                game_state.polygon_mode = GPU_POLYGON_LINE;
            } else {
                game_state.polygon_mode = GPU_POLYGON_FILL;
            }
        };
        table_add(layer.input_mapping_table, KEY_SWITCH_POLYGON_MODE, im);

        im = {};
        im.on_press = []() {
            if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                game_state.view_mode_flags &= ~VIEW_MODE_FLAG_COLLISION;
            } else {
                game_state.view_mode_flags |= VIEW_MODE_FLAG_COLLISION;
            }
        };
        table_add(layer.input_mapping_table, KEY_SWITCH_COLLISION_VIEW, im);

        im = {};
        im.on_press = []() {
            const auto layer = get_program_layer();
            if (layer->type == PROGRAM_LAYER_TYPE_EDITOR) {
                pop_program_layer();
                push_program_layer(&program_layer_game);
            } else if (layer->type == PROGRAM_LAYER_TYPE_GAME) {
                pop_program_layer();
                push_program_layer(&program_layer_editor);
            }
        };
        table_add(layer.input_mapping_table, KEY_SWITCH_EDITOR_MODE, im);
        
        im = {};
        im.on_press = []() {
            if (down(KEY_ALT)) {
                auto window = get_window();
                set_vsync(window, !window->vsync);
            }
        };
        table_add(layer.input_mapping_table, KEY_SWITCH_VSYNC, im);
    }

    {
        auto &layer = program_layer_game;
        layer.type = PROGRAM_LAYER_TYPE_GAME;
        layer.on_push  = on_push_game;
        layer.on_pop   = on_pop_game;
        layer.on_event = on_event_game;

        // Input_Mapping im;

        // im = {};
        // im.on_press = []() { };
        // table_add(layer.input_mapping_table, KEY_, im);
    }

    {
        auto &layer = program_layer_editor;
        layer.type = PROGRAM_LAYER_TYPE_EDITOR;
        layer.on_push  = on_push_editor;
        layer.on_pop   = on_pop_editor;
        layer.on_event = on_event_editor;

        Input_Mapping im;

        im = {};
        im.on_press = []() { if (down(KEY_CTRL)) save_current_level(); };
        table_add(layer.input_mapping_table, KEY_SAVE_LEVEL, im);

        im = {};
        im.on_press = []() { if (down(KEY_CTRL)) reload_current_level(); };
        table_add(layer.input_mapping_table, KEY_RELOAD_LEVEL, im);

        im = {};
        im.on_press = []() {
            auto window = get_window();
            if (!window->cursor_locked && ui.id_hot == UIID_NONE) {
                // @Cleanup: specify memory barrier target.
                gpu_memory_barrier();

                auto manager = get_entity_manager();
                auto e = get_entity(manager, gpu_picking_data->eid);
                if (e) mouse_pick_entity(e);
            }
        };
        table_add(layer.input_mapping_table, KEY_SELECT_ENTITY, im);

        im = {};
        im.on_press = mouse_unpick_entity;
        table_add(layer.input_mapping_table, KEY_UNSELECT_ENTITY, im);
    }

    {
        auto &layer = program_layer_console;
        layer.type = PROGRAM_LAYER_TYPE_CONSOLE;
        layer.on_push  = on_push_console;
        layer.on_pop   = on_pop_console;
        layer.on_event = on_event_console;

        Input_Mapping im;

        im = {};
        im.on_press = []() {
            auto layer = pop_program_layer();
            Assert(layer->type == PROGRAM_LAYER_TYPE_CONSOLE);
        };
        table_add(layer.input_mapping_table, KEY_OPEN_CONSOLE, im);

        im = {};
        im.on_press = []() {
            s32 delta = 1;

            if (down(KEY_CTRL))  delta *= get_console()->message_count;
            if (down(KEY_SHIFT)) delta *= 10;

            scroll_console(delta);
        };
        table_add(layer.input_mapping_table, KEY_UP, im);

        im = {};
        im.on_press = []() {
            s32 delta = -1;

            if (down(KEY_CTRL))  delta *= get_console()->message_count;
            if (down(KEY_SHIFT)) delta *= 10;

            scroll_console(delta);
        };
        table_add(layer.input_mapping_table, KEY_DOWN, im);

        im = {};
        im.on_press = []() { if (down(KEY_ALT)) context.log_level = LOG_VERBOSE; };
        table_add(layer.input_mapping_table, KEY_1, im);

        im = {};
        im.on_press = []() { if (down(KEY_ALT)) context.log_level = LOG_DEFAULT; };
        table_add(layer.input_mapping_table, KEY_2, im);

        im = {};
        im.on_press = []() { if (down(KEY_ALT)) context.log_level = LOG_WARNING; };
        table_add(layer.input_mapping_table, KEY_3, im);

        im = {};
        im.on_press = []() { if (down(KEY_ALT)) context.log_level = LOG_ERROR; };
        table_add(layer.input_mapping_table, KEY_4, im);
    }

    {
        
        auto &layer = program_layer_profiler;
        layer.type = PROGRAM_LAYER_TYPE_PROFILER;
        layer.on_push  = on_push_profiler;
        layer.on_pop   = on_pop_profiler;
        layer.on_event = on_event_profiler;
        
        Input_Mapping im;

        im = {};
        im.on_press = []() {
            auto layer = pop_program_layer();
            Assert(layer->type == PROGRAM_LAYER_TYPE_PROFILER);
        };
        table_add(layer.input_mapping_table, KEY_OPEN_PROFILER, im);
        
        im = {};
        im.on_press = switch_profiler_view;
        table_add(layer.input_mapping_table, KEY_SWITCH_PROFILER_VIEW, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                profiler->time_sort = 0;
            }
        };
        table_add(layer.input_mapping_table, KEY_1, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                profiler->time_sort = 1;
            }
        };
        table_add(layer.input_mapping_table, KEY_2, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                profiler->time_sort = 2;
            }
        };
        table_add(layer.input_mapping_table, KEY_3, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                profiler->time_sort = 3;
            }
        };
        table_add(layer.input_mapping_table, KEY_4, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                const auto delta = -1;
                profiler->selected_zone_index += delta;
            }
        };
        table_add(layer.input_mapping_table, KEY_UP, im);

        im = {};
        im.on_press = []() {
            auto profiler = get_profiler();
            if (profiler->view_mode == PROFILER_VIEW_RUNTIME) {
                const auto delta = 1;
                profiler->selected_zone_index += delta;
            }
        };
        table_add(layer.input_mapping_table, KEY_DOWN, im);
    }
}

// level

// Format of serialized text is reflection field name and its data, all separated by space.
// e.g: position 0.00 1.00 -2.50
// Serialized text allocated in temporary storage.
// In case of write given string parameter is a pointer to string to which serialzed text
// will be written.
// If serialize mode is read, string parameter is an array of string tokens that represent
// reflection field data which consists of field name and its tokens and their count
// depends on field type.
// e.g: u32, f32, bool etc. -> 1 | Vector4, Matrix2, etc. -> 4

static const auto level_header_token_camera = S("[CAMERA]");
static const auto level_header_token_entity = S("[ENTITY]");

template <typename T>
inline bool serialize_as_text(T *obj, const Reflection_Field *field, String *s, Serialize_Mode mode) {
    if (!obj) {
        log(LOG_ERROR, "Failed to serialze null object as text");
        return false;
    }

    if (!field) {
        log(LOG_ERROR, "Failed to serialze object as text with null reflection field");
        return false;
    }

    if (!s) {
        log(LOG_ERROR, "Failed to serialze object as text with null text data");
        return false;
    }

    if (mode == SERIALIZE_MODE_NONE) {
        log(LOG_ERROR, "Failed to serialze object as text with unknown serialize mode %d", mode);
        return false;
    }
    
    Assert(field->offset < sizeof(T));
    auto field_data = (u8 *)obj + field->offset;
    
    switch (field->type) {
    case REFLECTION_FIELD_TYPE_S8: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %d", field->name, *(s8 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u8)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_S16: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %d", field->name, *(s16 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (s16)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_S32: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %d", field->name, *(s32 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (s32)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_S64: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %lld", field->name, *(s64 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (s64)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_U8: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %u", field->name, *(u8 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u8)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_U16: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %u", field->name, *(u16 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u16)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_U32: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %u", field->name, *(u32 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u32)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_U64: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %llu", field->name, *(u32 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u64)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_F32: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %f", field->name, *(f32 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (f32)string_to_float(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_F64: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %f", field->name, *(f64 *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (f64)string_to_float(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_BOOL: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %d", field->name, *(bool *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (bool)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_CHAR: {
        if (mode == SERIALIZE_MODE_WRITE) {
            *s = tprint("%S %c", field->name, *(char *)field_data);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = s->data[0];
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_VECTOR2: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Vector2 *)field_data;
            *s = tprint("%S %f %f", field->name, v.x, v.y);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto x = (f32)string_to_float(s[1]);
            const auto y = (f32)string_to_float(s[2]);
            const auto v = Vector2(x, y);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_VECTOR3: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Vector3 *)field_data;
            *s = tprint("%S %f %f %f", field->name, v.x, v.y, v.z);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto x = (f32)string_to_float(s[1]);
            const auto y = (f32)string_to_float(s[2]);
            const auto z = (f32)string_to_float(s[3]);
            const auto v = Vector3(x, y, z);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_VECTOR4: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Vector4 *)field_data;
            *s = tprint("%S %f %f %f", field->name, v.x, v.y, v.z);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto x = (f32)string_to_float(s[1]);
            const auto y = (f32)string_to_float(s[2]);
            const auto z = (f32)string_to_float(s[3]);
            const auto w = (f32)string_to_float(s[4]);
            const auto v = Vector4(x, y, z, w);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_MATRIX2: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Matrix2 *)field_data;
            *s = tprint("%S %f %f %f %f", field->name, v[0], v[1], v[2], v[3]);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto a = (f32)string_to_float(s[1]);
            const auto b = (f32)string_to_float(s[2]);
            const auto c = (f32)string_to_float(s[3]);
            const auto d = (f32)string_to_float(s[4]);
            const auto v = Matrix2(a, b, c, d);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_MATRIX3: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Matrix3 *)field_data;
            *s = tprint("%S %f %f %f %f %f %f %f %f %f", field->name,
                        v[0], v[1], v[2],
                        v[3], v[4], v[5],
                        v[6], v[7], v[8]);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto a = (f32)string_to_float(s[1]);
            const auto b = (f32)string_to_float(s[2]);
            const auto c = (f32)string_to_float(s[3]);
            const auto d = (f32)string_to_float(s[4]);
            const auto e = (f32)string_to_float(s[5]);
            const auto f = (f32)string_to_float(s[6]);
            const auto g = (f32)string_to_float(s[7]);
            const auto h = (f32)string_to_float(s[8]);
            const auto i = (f32)string_to_float(s[9]);
            const auto v = Matrix3(a, b, c, d, e, f, g, h, i);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_MATRIX4: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Matrix4 *)field_data;
            *s = tprint("%S %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", field->name,
                        v[0],  v[1],  v[2],  v[3],
                        v[4],  v[5],  v[6],  v[7],
                        v[8],  v[9],  v[10], v[11],
                        v[12], v[13], v[14], v[15]);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto a = (f32)string_to_float(s[1]);
            const auto b = (f32)string_to_float(s[2]);
            const auto c = (f32)string_to_float(s[3]);
            const auto d = (f32)string_to_float(s[4]);
            const auto e = (f32)string_to_float(s[5]);
            const auto f = (f32)string_to_float(s[6]);
            const auto g = (f32)string_to_float(s[7]);
            const auto h = (f32)string_to_float(s[8]);
            const auto i = (f32)string_to_float(s[9]);
            const auto j = (f32)string_to_float(s[10]);
            const auto k = (f32)string_to_float(s[11]);
            const auto l = (f32)string_to_float(s[12]);
            const auto m = (f32)string_to_float(s[13]);
            const auto n = (f32)string_to_float(s[14]);
            const auto o = (f32)string_to_float(s[15]);
            const auto p = (f32)string_to_float(s[16]);
            const auto v = Matrix4(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_QUATERNION: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Quaternion *)field_data;
            *s = tprint("%S %f %f %f %f", field->name, v.x, v.y, v.z, v.w);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto x = (f32)string_to_float(s[1]);
            const auto y = (f32)string_to_float(s[2]);
            const auto z = (f32)string_to_float(s[3]);
            const auto w = (f32)string_to_float(s[4]);
            const auto v = Vector4(x, y, z, w);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_STRING: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(String *)field_data;
            *s = tprint("%S %S", field->name, v);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = copy_string(trim(s[1]));
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_ATOM: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Atom *)field_data;
            *s = tprint("%S %S", field->name, get_string(v));
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = make_atom(trim(s[1]));
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_PID: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(Pid *)field_data;
            *s = tprint("%S %u", field->name, v);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto v = (u32)string_to_integer(s[1]);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    case REFLECTION_FIELD_TYPE_AABB: {
        if (mode == SERIALIZE_MODE_WRITE) {
            const auto v = *(AABB *)field_data;
            *s = tprint("%S %f %f %f  %f %f %f",
                        field->name, v.c.x, v.c.y, v.c.z, v.r.x, v.r.y, v.r.z);
            return true;
        } else if (mode == SERIALIZE_MODE_READ) {
            Assert(field->name == s[0]);
            const auto cx = (f32)string_to_float(s[1]);
            const auto cy = (f32)string_to_float(s[2]);
            const auto cz = (f32)string_to_float(s[3]);
            const auto rx = (f32)string_to_float(s[4]);
            const auto ry = (f32)string_to_float(s[5]);
            const auto rz = (f32)string_to_float(s[6]);
            const auto v = make_aabb(cx, cy, cz, rx, ry, rz);
            copy(field_data, &v, sizeof(v));
            return true;
        }
    }
    }

    log(LOG_ERROR, "Failed to serialze (mode %d) reflection field 0x%X for object 0x%X", mode, field, obj);
    return false;
}

static Level *current_level;

Level *new_level(String path) {
    auto contents = read_text_file(path, __temporary_allocator);
    if (!contents) return null;
    const auto name = get_file_name_no_ext(path);
    const auto atom = make_atom(name);
    return new_level(atom, contents);
}

Level *new_level(Atom name, String contents) {
    auto &level = level_table[name];
    level.name = name;

    if (level.entity_manager) reset(level.entity_manager);
    else level.entity_manager = new_entity_manager();

    Text_File_Handler text;
    text.allocator = __temporary_allocator;
    
    defer { reset(&text); };
    init_from_memory(&text, contents);

    static const auto get_entity_reflection_field = [](String name) -> const Reflection_Field * {
        for (u32 i = 0; i < Reflection_Field_Count(Entity); ++i) {
            const auto field = Reflection_Field_At(Entity, i);
            if (field->name == name) return field;
        }
        return null;
    };

    static const auto get_camera_reflection_field = [](String name) -> const Reflection_Field * {
        for (u32 i = 0; i < Reflection_Field_Count(Camera); ++i) {
            const auto field = Reflection_Field_At(Camera, i);
            if (field->name == name) return field;
        }
        return null;
    };
      
    Entity *e = null;
    auto camera = &level.entity_manager->camera;
    bool parse_entity = false;
    
    while (1) {
        auto line = read_line(&text);        
        if (!line) break;
        
        const auto delims = S(" ");
        auto tokens = split(line, delims);
        if (!tokens.count) continue;

        const auto header = trim(tokens[0]);
        if (header == level_header_token_entity) {
            if (e && e->type == E_PLAYER) level.entity_manager->player = e->eid; 
            if (e && e->type == E_SKYBOX) level.entity_manager->skybox = e->eid;
            
            e = New_Entity(level.entity_manager, E_NONE);
            parse_entity = true;
            continue;
        } else if (header == level_header_token_camera) {
            parse_entity = false;
            continue;
        }

        // Field parsing.
        
        if (tokens.count == 1) continue; // empty field data

        if (parse_entity) {
            const auto field = get_entity_reflection_field(tokens[0]);
            serialize_as_text(e, field, tokens.items, SERIALIZE_MODE_READ);
        } else {
            const auto field = get_camera_reflection_field(tokens[0]);
            serialize_as_text(camera, field, tokens.items, SERIALIZE_MODE_READ);
        }
    }

    on_viewport_resize(*camera, screen_viewport);
    update_matrices(*camera);

    return &level;
}

Level *get_level(Atom name) {
    return table_find(level_table, name);
}

Level *get_current_level() {
    return current_level;
}

void set_current_level(Atom name) {
    auto level = get_level(name);
    if (!level) {
        log(LOG_ERROR, "Failed to set current level to unknown level %S", get_string(name));
        return;
    }

    current_level = level;
}

static inline String make_level_path(Atom name) {
    const auto level_name = get_string(name);
    const auto level_path = tprint("%S%S.%S", DIR_LEVELS, level_name, LEVEL_EXT);
    return level_path;
}

void reload_current_level() {
    START_TIMER(0);

    const auto level = get_current_level();
    if (!level) {
        log(LOG_ERROR, "Failed to reload current level as it does not exist");
        return;
    }

    const auto level_path = make_level_path(level->name);
    auto new_lvl = new_level(level_path);
    Assert(new_lvl->name == level->name);
    
    log("Reloaded %S %.2fms", level_path, CHECK_TIMER_MS(0));
    screen_report("Reloaded current level");
}

void save_current_level() {
    START_TIMER(0);
    
    const auto level = get_current_level();
    if (!level) {
        log(LOG_ERROR, "Failed to save current level as it does not exist");
        return;
    }

    const auto level_path = make_level_path(level->name);
    
    Archive archive;
    defer { reset(archive); };

    if (!start_write(archive, level_path, true)) return;

    const auto mark = get_temporary_storage_mark();
    defer { set_temporary_storage_mark(mark); };
    
    String_Builder sb;
    sb.allocator = __temporary_allocator;

    append(sb, level_header_token_camera);
    append(sb, "\n");

    const auto camera = &level->entity_manager->camera;
    for (u32 i = 0; i < Reflection_Field_Count(Camera); ++i) {
        const auto field = Reflection_Field_At(Camera, i);

        String text;
        if (serialize_as_text(camera, field, &text, SERIALIZE_MODE_WRITE)) {  
            append(sb, text);
            append(sb, "\n");
        }
    }
        
    append(sb, "\n");
        
    For (level->entity_manager->entities) {
        if (it.type == E_NONE) continue;
        
        append(sb, level_header_token_entity);
        append(sb, "\n");
        
        for (u32 i = 0; i < Reflection_Field_Count(Entity); ++i) {
            const auto field = Reflection_Field_At(Entity, i);

            String text;
            if (serialize_as_text(&it, field, &text, SERIALIZE_MODE_WRITE)) {
                append(sb, text);
                append(sb, "\n");
            }
        }
        
        append(sb, "\n");
    }

    const auto s = builder_to_string(sb);
    serialize(archive, s.data, s.size);

    log("Saved %S %.2fms", level_path, CHECK_TIMER_MS(0));
    screen_report("Saved current level");
}

static const Atom player_move_flip_book_lut[DIRECTION_COUNT] = {
    ATOM("player_move_back"),
    ATOM("player_move_right"),
    ATOM("player_move_left"),
    ATOM("player_move_forward"),
};

static const Atom player_idle_texture_lut[DIRECTION_COUNT] = {
    ATOM("player_idle_back"),
    ATOM("player_idle_right"),
    ATOM("player_idle_left"),
    ATOM("player_idle_forward"),
};

void on_push_game(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_GAME);
    screen_report("Game");
}

void on_pop_game(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_GAME);

    const auto audio_player = get_audio_player();
    For (audio_player->playing_sounds) stop_sound(it);

    // @Todo: make separate entity managers for game and editor.
    auto manager = get_entity_manager();
    auto player = get_player(manager);
    player->velocity = Vector3_zero;

    get_material(player->material)->diffuse_texture = player_idle_texture_lut[player->move_direction];
}

bool on_event_game(Program_Layer *layer, const Window_Event *e) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_GAME);
    
    auto handled = on_event_input_mapping(layer, e);
    if (handled) return handled;
    
    return handled;
}

void simulate_game() {
    Profile_Zone(__func__);

    For (program_layer_stack) {
        if (it->type == PROGRAM_LAYER_TYPE_EDITOR) return;
    }
    
    const f32 dt = get_delta_time();
    
    auto window  = get_window();
    auto input   = get_input_table();
    auto layer   = get_program_layer();
    auto manager = get_entity_manager();
    auto player  = get_player(manager);
    auto &camera = manager->camera;
    auto &skybox = manager->skybox;

    // Update camera.
    {
        if (game_state.camera_behavior == STICK_TO_PLAYER) {
            camera.position = player->position + player->camera_offset;
        } else if (game_state.camera_behavior == FOLLOW_PLAYER) {
            const auto dead_zone     = player->camera_dead_zone;
            const auto dead_zone_min = camera.position - dead_zone * 0.5f;
            const auto dead_zone_max = camera.position + dead_zone * 0.5f;
            const auto desired_eye   = player->position + player->camera_offset;

            Vector3 target_eye;
            if (desired_eye.x < dead_zone_min.x)
                target_eye.x = desired_eye.x + dead_zone.x * 0.5f;
            else if (desired_eye.x > dead_zone_max.x)
                target_eye.x = desired_eye.x - dead_zone.x * 0.5f;
            else
                target_eye.x = camera.position.x;

            if (desired_eye.y < dead_zone_min.y)
                target_eye.y = desired_eye.y + dead_zone.y * 0.5f;
            else if (desired_eye.y > dead_zone_max.y)
                target_eye.y = desired_eye.y - dead_zone.y * 0.5f;
            else
                target_eye.y = camera.position.y;

            if (desired_eye.z < dead_zone_min.z)
                target_eye.z = desired_eye.z + dead_zone.z * 0.5f;
            else if (desired_eye.z > dead_zone_max.z)
                target_eye.z = desired_eye.z - dead_zone.z * 0.5f;
            else
                target_eye.z = camera.position.z;

            camera.position = Lerp(camera.position, target_eye, player->camera_follow_speed * dt);
        }

        const auto look_at = Vector3(camera.position.x, camera.position.y, camera.position.z + 1.0f);
        camera.look_at_position = look_at;
    }

    update_matrices(camera);
    
    // Update player.
    {
        const auto speed = player->move_speed * dt;
        auto velocity = Vector3_zero;

        if (layer->type == PROGRAM_LAYER_TYPE_GAME) {
            if (game_state.player_movement_behavior == MOVE_INDEPENDENT) {
                if (down(KEY_D)) {
                    velocity.x = speed;
                    player->move_direction = EAST;
                }

                if (down(KEY_A)) {
                    velocity.x = -speed;
                    player->move_direction = WEST;
                }

                if (down(KEY_W)) {
                    velocity.z = speed;
                    player->move_direction = NORTH;
                }

                if (down(KEY_S)) {
                    velocity.z = -speed;
                    player->move_direction = SOUTH;
                }
            } else if (game_state.player_movement_behavior == MOVE_RELATIVE_TO_CAMERA) {
                auto &camera = manager->camera;
                const Vector3 camera_forward = forward(camera.yaw, camera.pitch);
                const Vector3 camera_right = normalize(cross(camera.up_vector, camera_forward));

                if (down(KEY_D)) {
                    velocity += speed * camera_right;
                    player->move_direction = EAST;
                }

                if (down(KEY_A)) {
                    velocity -= speed * camera_right;
                    player->move_direction = WEST;
                }

                if (down(KEY_W)) {
                    velocity += speed * camera_forward;
                    player->move_direction = NORTH;
                }

                if (down(KEY_S)) {
                    velocity -= speed * camera_forward;
                    player->move_direction = SOUTH;
                }
            }
        }
        
        player->velocity = truncate(velocity, speed);

        auto &player_aabb = player->aabb;

        auto material = get_material(player->material);
        if (player->velocity == Vector3_zero) {
            material->diffuse_texture = player_idle_texture_lut[player->move_direction];
        } else {
            player->move_flip_book = player_move_flip_book_lut[player->move_direction];

            auto flip_book = get_flip_book(player->move_flip_book);
            update(flip_book, dt);
            
            auto frame = get_current_frame(flip_book);
            material->diffuse_texture = frame->texture;
        }
        
        if (player->velocity == Vector3_zero) {
            stop_sound(player->move_sound);
        } else {
            play_sound(player->move_sound);
        }

        set_audio_listener_position(player->position);
    }

    For (manager->entities) {
        auto ea = &it;
        
        For (manager->entities) {
            auto eb = &it;

            if (ea->eid != eb->eid) {
                auto ba = AABB { .c = ea->aabb.c + ea->velocity * dt, .r = ea->aabb.r };
                auto bb = AABB { .c = eb->aabb.c + eb->velocity * dt, .r = eb->aabb.r };

                if (overlap(ba, bb)) {
                    ea->bits |= E_OVERLAP_BIT;
                    eb->bits |= E_OVERLAP_BIT;
                    
                    ea->velocity = Vector3_zero;
                    eb->velocity = Vector3_zero;
                }
            }
        }
    }

    For (manager->entities) {
        it.position += it.velocity * dt;
        it.object_to_world = make_transform(it.position, it.orientation, it.scale);
        move_aabb_along_with_entity(&it);
    }
    
    For_Entities (manager->entities, E_SOUND_EMITTER) {
        if (it.sound_play_spatial) {
            play_sound(it.sound, it.position);
        } else {
            play_sound(it.sound);
        }
    }
}

Entity_Manager *get_entity_manager() {
    auto level = get_current_level();
    Assert(level->entity_manager);
    return level->entity_manager;
}

Entity_Manager *new_entity_manager() {
    auto &manager = array_add(entity_managers) = {};
    reset(&manager);
    return &manager;
}

void reset(Entity_Manager *manager) {
    array_clear(manager->entities);
    array_clear(manager->entities_to_delete);
    array_clear(manager->free_entities);

    manager->camera = {};
    manager->player = 0;
    manager->skybox = 0;

    // Add default nil entity.
    array_add(manager->entities, {});
}

void post_frame_cleanup(Entity_Manager *manager) {
    For (manager->entities_to_delete) {
        auto e = get_entity(manager, it);
        e->bits |= E_DELETED_BIT;
        
        array_add(manager->free_entities, it);
    }

    array_clear(manager->entities_to_delete);

    For (manager->entities) {
        it.bits &= ~E_OVERLAP_BIT;
    }
}

Pid new_entity(Entity_Manager *manager, Entity_Type type) {
    auto &entities      = manager->entities;
    auto &free_entities = manager->free_entities;

    u32 index      = 0;
    u16 generation = 0;
    
    if (free_entities.count) {
        const auto eid = array_pop(free_entities);
        index      = get_eid_index(eid);
        generation = get_eid_generation(eid) + 1;
    } else {
        index      = entities.count;
        generation = 1;
        array_add(entities);
    }

    auto &e = entities[index] = {};
    e.type     = type;
    e.eid      = make_eid(index, generation);
    e.scale    = Vector3(1.0f);
    e.uv_scale = Vector2(1.0f);
    
    return e.eid;
}

void delete_entity(Entity_Manager *manager, Pid eid) {
    array_add(manager->entities_to_delete, eid);
}

Entity *get_entity(Entity_Manager *manager, Pid eid) {
    auto &entities = manager->entities;

    const auto index      = get_eid_index(eid);
    const auto generation = get_eid_generation(eid);
    
    if (index > 0 && index < entities.count && generation) {
        auto &e = entities[index];
        if (e.eid == eid && !(e.bits & E_DELETED_BIT)) return &e;
    }

    log(LOG_ERROR, "Failed to get entity by eid %u (%u %u) from entity manager 0x%X", eid, index, generation, manager);
      
    return &entities[0];
}

Entity *get_player(Entity_Manager *manager) {
    return get_entity(manager, manager->player);
}

Entity *get_skybox(Entity_Manager *manager) {
    return get_entity(manager, manager->skybox);
}

void move_aabb_along_with_entity(Pid eid) {
    auto manager = get_entity_manager();
    auto e = get_entity(manager, eid);
    if (!e) return;
    move_aabb_along_with_entity(e);
}

void move_aabb_along_with_entity(Entity *e) {
    Assert(e);
    
    const auto dt = get_delta_time();
    e->aabb.c += e->velocity * dt;
}
