#include "pch.h"
#include "game.h"
#include "entity_manager.h"
#include "world.h"
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
        case ASSET_TYPE_MATERIAL:  new_material  (entry.name, make_string(entry.buffer)); break;
        case ASSET_TYPE_SOUND:     new_sound     (entry.name, entry.buffer); break;
        case ASSET_TYPE_FLIP_BOOK: new_flip_book (entry.name, make_string(entry.buffer)); break;
        case ASSET_TYPE_MESH:      new_mesh      (entry.name, entry.buffer); break;
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

        global_materials.missing = new_material(S("missing.material"), make_string((u8 *)missing_material_source));
    }
     
    {
        #include "missing_mesh.h"

        const auto buffer = make_buffer((void *)missing_mesh_obj, cstring_count(missing_mesh_obj));
        global_meshes.missing = new_mesh(S("missing.obj"), buffer);
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

static const String player_move_flip_book_lut[DIRECTION_COUNT] = {
    S("player_move_back"),
    S("player_move_right"),
    S("player_move_left"),
    S("player_move_forward"),
};

static const Atom player_idle_texture_lut[DIRECTION_COUNT] = {
    ATOM("player_idle_back"),
    ATOM("player_idle_right"),
    ATOM("player_idle_left"),
    ATOM("player_idle_forward"),
};

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

        if (e->press) set_bit  (keys, key);
        else          clear_bit(keys, key);
        
        break;
    }
    case WINDOW_EVENT_MOUSE_MOVE: {
        input->mouse_offset_x =  e->x;
        input->mouse_offset_y = -e->y;

        auto window = get_window();
        if (window->cursor_locked) {
            const auto x = input->mouse_x = window->width  / 2;
            const auto y = input->mouse_y = window->height / 2;
            set_cursor(window, x, y);
        } else {
            get_cursor(window, &input->mouse_x, &input->mouse_y);
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
        if (e->type == WINDOW_EVENT_MOUSE_BUTTON && input->mouse_y < 0) break;

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

        // im = {};
        // im.on_press = []() { if (down(KEY_CTRL)) save_current_level(); };
        // table_add(layer.input_mapping_table, KEY_SAVE_LEVEL, im);

        // im = {};
        // im.on_press = []() { if (down(KEY_CTRL)) load_level(); };
        // table_add(layer.input_mapping_table, KEY_RELOAD_LEVEL, im);

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

// void save_level(Game_World &world) {
//     START_TIMER(save);
    
//     auto path = tprint("%S%S", PATH_LEVEL(""), world.name);
    
//     File file = open_file(path, FILE_WRITE_BIT);
//     defer { close_file(file); };
    
//     if (file == FILE_NONE) {
//         log("Level %S does not exist, creating new one", path);
//         file = open_file(path, FILE_NEW_BIT | FILE_WRITE_BIT);
//         if (file == FILE_NONE) {
//             log(LOG_MINIMAL, "Failed to create new level %S", path);
//             return;
//         }
//     }

//     write_file(file, world.MAX_NAME_SIZE, &world.name);
    
//     write_file(file, _sizeref(world.player));
//     write_file(file, _sizeref(world.camera));
//     write_file(file, _sizeref(world.ed_camera));
//     write_file(file, _sizeref(world.skybox));

//     // write_sparse_array(file, &world.static_meshes);
//     // write_sparse_array(file, &world.point_lights);
//     // write_sparse_array(file, &world.direct_lights);
//     // write_sparse_array(file, &world.sound_emitters_2d);
//     // write_sparse_array(file, &world.sound_emitters_3d);
//     // write_sparse_array(file, &world.portals);
//     // write_sparse_array(file, &world.aabbs);
    
//     log("Saved level %S in %.2fms", path, CHECK_TIMER_MS(save));
//     screen_report("Saved level %S", path);
// }

// static For_Result cb_init_entity_after_level_load(Entity *e, void *user_data) {
//     auto *world = (Game_World *)user_data;

//     if (e->eid != EID_NONE) {        
//         // e->draw_data.eid_offset = get_head_position(R_eid_alloc_range);
//         // *(eid *)r_alloc(R_eid_alloc_range, sizeof(eid)) = e->eid;
//     }

//     if (e->eid > eid_global_counter) {
//         eid_global_counter = e->eid + 1;
//     }
    
//     return CONTINUE;
// }

// void load_level(Game_World &world, String path) {
//     START_TIMER(0);

//     File file = open_file(path, FILE_READ_BIT);
//     defer { close_file(file); };
    
//     if (file == FILE_NONE) {
//         log(LOG_MINIMAL, "Failed to open level %S", path);
//         return;
//     }

//     read_file(file, world.MAX_NAME_SIZE, &world.name);
    
//     read_file(file, _sizeref(world.player));
//     read_file(file, _sizeref(world.camera));
//     read_file(file, _sizeref(world.ed_camera));
//     read_file(file, _sizeref(world.skybox));

//     // read_sparse_array(file, &world.static_meshes);
//     // read_sparse_array(file, &world.point_lights);
//     // read_sparse_array(file, &world.direct_lights);
//     // read_sparse_array(file, &world.sound_emitters_2d);
//     // read_sparse_array(file, &world.sound_emitters_3d);
//     // read_sparse_array(file, &world.portals);
//     // read_sparse_array(file, &world.aabbs);

//     for_each_entity(world, cb_init_entity_after_level_load, &world);
    
//     log("Loaded level %S in %.2fms", path, CHECK_TIMER_MS(0));
//     screen_report("Loaded level %S", path);
// }

void on_push_game(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_GAME);
    screen_report("Game");
}

void on_pop_game(Program_Layer *layer) {
    Assert(layer->type == PROGRAM_LAYER_TYPE_GAME);

    auto manager = get_entity_manager();

    auto player = get_player(manager);
    player->velocity = Vector3_zero;
    // @Cleanup: it would be better to have an array of currently playing sounds.
    stop_sound(player->move_sound);
    get_material(player->material)->diffuse_texture = get_texture(player_idle_texture_lut[player->move_direction]);
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
            camera.look_at_position = camera.position + forward(camera.yaw, camera.pitch);
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
            camera.look_at_position = camera.position + forward(camera.yaw, camera.pitch);
        }
    }
    
    // This can be used to force camera to look at specific position.
    // camera.look_at_position = direction(camera.position, fixed_position);

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
            material->diffuse_texture = get_texture(player_idle_texture_lut[player->move_direction]);
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

void switch_campaign(Level_Set *set) {
    current_level_set = set;

    if (!set->campaign_state) {
        auto cs = New(Campaign_State);
        cs->campaign_name = copy_string(set->name);
        set->campaign_state = cs;
    }

    init_level_general(false);
}

void init_level_general(bool reset) {
    auto cs = get_campaign_state();
    if (cs->entity_manager) {
        if (reset) ::reset(cs->entity_manager);
    } else {
        cs->entity_manager = new_entity_manager();
        reset = true;
    }

    auto manager = cs->entity_manager;
    // @Todo
}

Level_Set *get_level_set() {
    auto set = current_level_set;
    Assert(set);
    return set;
}

Campaign_State *get_campaign_state() {
    auto set = get_level_set();
    Assert(set->campaign_state)
    return set->campaign_state;
}

Entity_Manager *get_entity_manager() {
    auto cs = get_campaign_state();
    Assert(cs->entity_manager);
    return cs->entity_manager;
}

Entity_Manager *new_entity_manager() {
    auto &manager = array_add(entity_managers);
    new (&manager) Entity_Manager;

    // @Cleanup: reallocate some entity count here?
    // Add default nil entity.
    array_add(manager.entities, {});
    
    return &manager;
}

void reset(Entity_Manager *manager) {
    array_clear(manager->entities);
    array_clear(manager->entities_to_delete);
    array_clear(manager->free_entities);

    *manager = {};
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

    auto &e = entities[index];
    e.type     = type;
    e.bits    &= ~E_DELETED_BIT;
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
