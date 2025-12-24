#include "pch.h"
#include "game.h"
#include "entity_manager.h"
#include "world.h"
#include "profile.h"
#include "input.h"
#include "input_stack.h"
#include "game_pak.h"
#include "file_system.h"
#include "window.h"
#include "editor.h"
#include "console.h"
#include "render.h"
#include "shader_binding_model.h"
#include "ui.h"
#include "render_target.h"
#include "viewport.h"
#include "material.h"
#include "texture.h"
#include "flip_book.h"
#include "audio_player.h"

void game_logger_proc(String message, String ident, Log_Level level, void *logger_data) {
    if (ident && ident == LOG_IDENT_GL) return;
    
    Assert(logger_data);
    auto logger = (Game_Logger_Data *)logger_data;

    String s;
    if (ident) {
        s = tprint("[%S] %S\n", ident, message);
    } else {
        s = tprint("%S\n", message);
    }

    append(logger->messages, s);
    add_to_console_history(s);
}

void flush_game_logger() {
    auto mega_message = builder_to_string(game_logger_data.messages);
    print(mega_message);
}

void on_window_resize(u16 width, u16 height) {
    resize(screen_viewport, width, height);

    auto manager = get_entity_manager();
    on_viewport_resize(manager->camera, screen_viewport);
    
    on_console_viewport_resize(screen_viewport.width, screen_viewport.height);
}

void init_asset_storages() {
    add_directory_files(&texture_catalog,   PATH_TEXTURE(""),   true);
    add_directory_files(&flip_book_catalog, PATH_FLIP_BOOK(""), true);
    add_directory_files(&material_catalog,  PATH_MATERIAL(""),  true);
    
    table_realloc(texture_table,       texture_catalog.entries.count);
    table_realloc(material_table,      64);
    table_realloc(triangle_mesh_table, 64);
    table_realloc(flip_book_table,     64);
    table_realloc(font_atlas_table,    16);
}

void load_game_assets() {
#if DEVELOPER
    // Shaders.
    {
        static auto load_shader = [](const File_Callback_Data *data) {
            START_TIMER(0);

            const auto path = data->path;
            const auto load_header = *(bool *)data->user_data;
    
            if (load_header && is_shader_header_path(path)) {
                if (!new_shader_file(path)) return;
            } else if (!load_header && is_shader_source_path(path)) {
                if (!new_shader(path)) return;
            } else {
                // Skip other file extensions.
                return;
            }

            log("Created %S", path, CHECK_TIMER_MS(0));
        };
        
        bool load_header = true;
        visit_directory(PATH_SHADER(""), load_shader, true, &load_header);
    
        load_header = false;
        visit_directory(PATH_SHADER(""), load_shader, true, &load_header);
    }
    
    // Textures.
    {
        For (texture_catalog.entries) {
            if (new_texture(it.path)) log("Created %S", it.path);
        }
    }

    // Meshes.
    {
        visit_directory(PATH_MESH(""), [] (const File_Callback_Data *data) {
            if (new_mesh(data->path)) log("Created %S", data->path);
        });
    }

    // Sounds.
    {
        auto &sound_catalog = get_audio_player()->sound_catalog;
        For (sound_catalog.entries) {
            if (new_sound(it.path)) log("Created %S", it.path);
        }
    }

    // Flip books.
    {
        For (flip_book_catalog.entries) {
            if (new_flip_book(it.path)) log("Created %S", it.path);
        }
    }

    // Materials.
    {
        For (material_catalog.entries) {
            if (new_material(it.path)) log("Created %S", it.path);
        }
    }
    
    save_game_pak(GAME_PAK_PATH);  
#endif
    
    load_game_pak(GAME_PAK_PATH);

    // Materials.
    // {
    //     auto entity_shader = get_shader(S("entity"));
    
    //     auto cube = new_material(S("cube"), entity_shader);
    //     cube->diffuse_texture = get_texture(S("stone"));
    
    //     auto ground = new_material(S("ground"), entity_shader);
    //     ground->diffuse_texture = get_texture(S("grass"));
    
    //     auto player = new_material(S("player"), entity_shader);
    //     player->diffuse_texture = get_texture(S("player_idle_back"));
    //     player->use_blending    = true;
    
    //     auto tower = new_material(S("tower"), entity_shader);
    //     tower->diffuse_texture = get_texture(S("stone"));

    //     auto skybox = new_material(S("skybox"), get_shader(S("skybox")));
    //     skybox->diffuse_texture = get_texture(S("skybox"));

    //     new_material(S("frame_buffer"), get_shader(S("frame_buffer")));
    //     new_material(S("geometry"),     get_shader(S("geometry")));
    //     new_material(S("outline"),      get_shader(S("outline")));
    //     new_material(S("ui_element"),   get_shader(S("ui_element")));
    //     new_material(S("ui_text"),      get_shader(S("ui_text")));
    // }

    // Font atlases.
    {
        {
            #include "better_vcr_16.h"
            constexpr String name = S("better_vcr_16");

            auto texture = get_texture(name);        
            auto &atlas = font_atlas_table[name];
            atlas.name           = name;
            atlas.texture        = texture;
            atlas.start_charcode = better_vcr_16_start_charcode;
            atlas.end_charcode   = atlas.start_charcode + carray_count(better_vcr_16_cdata) - 1;
            atlas.ascent         = better_vcr_16_ascent;
            atlas.descent        = better_vcr_16_descent;
            atlas.line_gap       = better_vcr_16_line_gap;
            atlas.line_height    = better_vcr_16_line_height;
            atlas.px_height      = 16;
            atlas.px_h_scale     = better_vcr_16_px_h_scale;
            atlas.space_xadvance = better_vcr_16_cdata[0].xadvance;
            atlas.glyphs         = New(Glyph, carray_count(better_vcr_16_cdata));

            copy(atlas.glyphs, better_vcr_16_cdata, sizeof(better_vcr_16_cdata));

            global_font_atlases.main_small = &atlas;
        }

        {
            #include "better_vcr_24.h"
            constexpr String name = S("better_vcr_24");

            auto texture = get_texture(name);        
            auto &atlas = font_atlas_table[name];
            atlas.name           = name;
            atlas.texture        = texture;
            atlas.start_charcode = better_vcr_24_start_charcode;
            atlas.end_charcode   = atlas.start_charcode + carray_count(better_vcr_16_cdata) - 1;
            atlas.ascent         = better_vcr_24_ascent;
            atlas.descent        = better_vcr_24_descent;
            atlas.line_gap       = better_vcr_24_line_gap;
            atlas.line_height    = better_vcr_24_line_height;
            atlas.px_height      = 24;
            atlas.px_h_scale     = better_vcr_24_px_h_scale;
            atlas.space_xadvance = better_vcr_24_cdata[0].xadvance;
            atlas.glyphs         = New(Glyph, carray_count(better_vcr_24_cdata));

            copy(atlas.glyphs, better_vcr_24_cdata, sizeof(better_vcr_24_cdata));
        
            global_font_atlases.main_medium = &atlas;
        }
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

void init_input() {
    {
        auto &layer = input_layers[INPUT_LAYER_GAME];
        layer.type = INPUT_LAYER_GAME;
        layer.on_input = on_game_input;
        layer.on_push  = on_game_push;
        layer.on_pop   = on_game_pop;
    }

    {
        auto &layer = input_layers[INPUT_LAYER_EDITOR];
        layer.type = INPUT_LAYER_EDITOR;
        layer.on_input = on_editor_input;
        layer.on_push  = on_editor_push;
        layer.on_pop   = on_editor_pop;

        push_input_layer(layer);
    }

    {
        auto &layer = input_layers[INPUT_LAYER_CONSOLE];
        layer.type = INPUT_LAYER_CONSOLE;
        layer.on_input = on_console_input;
    }

    {
        auto &layer = input_layers[INPUT_LAYER_PROFILER];
        layer.type = INPUT_LAYER_PROFILER;
        layer.on_input = on_profiler_input;
    }    
}

static constexpr String player_move_flip_book_lut[DIRECTION_COUNT] = {
    S("player_move_back"),
    S("player_move_right"),
    S("player_move_left"),
    S("player_move_forward"),
};

static constexpr String player_idle_texture_lut[DIRECTION_COUNT] = {
    S("player_idle_back"),
    S("player_idle_right"),
    S("player_idle_left"),
    S("player_idle_forward"),
};

void on_game_push() {
    screen_report("Game");
}

void on_game_pop() {
    auto manager = get_entity_manager();

    {
        auto player = get_player(manager);
        player->velocity = Vector3_zero;
        // @Cleanup: it would be better to have an array of currently playing sounds.
        stop_sound(player->move_sound);
        get_material(player->material)->diffuse_texture = get_texture(player_idle_texture_lut[player->move_direction]);

    }
    
    mouse_unpick_entity();
}

void on_game_input(const Window_Event *e) {
    auto window = get_window();

    switch (e->type) {
	case WINDOW_EVENT_KEYBOARD: {
        const auto key   = e->key_code;
        const auto press = e->input_bits & WINDOW_EVENT_PRESS_BIT;
        
        if (press && key == KEY_OPEN_CONSOLE) {
            open_console();
        } else if (press && key == KEY_OPEN_PROFILER) {
            open_profiler();
        } else if (press && key == KEY_SWITCH_POLYGON_MODE) {
            if (game_state.polygon_mode == POLYGON_FILL) {
                game_state.polygon_mode = POLYGON_LINE;
            } else {
                game_state.polygon_mode = POLYGON_FILL;
            }
        } else if (press && key == KEY_SWITCH_COLLISION_VIEW) {
            if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                game_state.view_mode_flags &= ~VIEW_MODE_FLAG_COLLISION;
            } else {
                game_state.view_mode_flags |= VIEW_MODE_FLAG_COLLISION;
            }
        } else if (press && key == KEY_1) {
            game_state.camera_behavior = (Camera_Behavior)(((s32)game_state.camera_behavior + 1) % 3);
        } else if (press && key == KEY_2) {
            game_state.player_movement_behavior = (Player_Movement_Behavior)(((s32)game_state.player_movement_behavior + 1) % 2);
        }

        break;
	}
    }
}

// template <typename T>
// static void read_sparse_array(File file, Sparse<T> *array) {
//     // @Cleanup: read sparse array directly with replace or add read data to existing one?

//     read_file(file, _sizeref(array->count));

//     s32 capacity = 0;
//     read_file(file, _sizeref(capacity));
//     Assert(capacity <= array->capacity);

//     read_file(file, capacity * sizeof(T),   array->items);
//     read_file(file, capacity * sizeof(s32), array->dense);
//     read_file(file, capacity * sizeof(s32), array->sparse);
// }

// template <typename T>
// static void write_sparse_array(File file, Sparse<T> *array) {
//     write_file(file, _sizeref(array->count));
//     write_file(file, _sizeref(array->capacity));
//     write_file(file, array->capacity * sizeof(T),   array->items);
//     write_file(file, array->capacity * sizeof(s32), array->dense);
//     write_file(file, array->capacity * sizeof(s32), array->sparse);
// }

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

void simulate_game() {
    Profile_Zone(__func__);

    const f32 dt = get_delta_time();
    
    auto window      = get_window();
    auto input       = get_input_table();
    auto input_layer = get_current_input_layer();
    auto manager     = get_entity_manager();
    auto player      = get_player(manager);
    auto &camera     = manager->camera;
    auto &skybox     = manager->skybox;

    // Update camera.
    {
        if (game_state.camera_behavior == STICK_TO_PLAYER) {
            camera.position = player->position + player->camera_offset;
            camera.look_at_position = camera.position + forward(camera.yaw, camera.pitch);
        } else if (game_state.camera_behavior == FOLLOW_PLAYER) {
            const Vector3 camera_dead_zone = player->camera_dead_zone;
            const Vector3 dead_zone_min = camera.position - camera_dead_zone * 0.5f;
            const Vector3 dead_zone_max = camera.position + camera_dead_zone * 0.5f;
            const Vector3 desired_camera_eye = player->position + player->camera_offset;

            Vector3 target_eye;
            if (desired_camera_eye.x < dead_zone_min.x)
                target_eye.x = desired_camera_eye.x + camera_dead_zone.x * 0.5f;
            else if (desired_camera_eye.x > dead_zone_max.x)
                target_eye.x = desired_camera_eye.x - camera_dead_zone.x * 0.5f;
            else
                target_eye.x = camera.position.x;

            if (desired_camera_eye.y < dead_zone_min.y)
                target_eye.y = desired_camera_eye.y + camera_dead_zone.y * 0.5f;
            else if (desired_camera_eye.y > dead_zone_max.y)
                target_eye.y = desired_camera_eye.y - camera_dead_zone.y * 0.5f;
            else
                target_eye.y = camera.position.y;

            if (desired_camera_eye.z < dead_zone_min.z)
                target_eye.z = desired_camera_eye.z + camera_dead_zone.z * 0.5f;
            else if (desired_camera_eye.z > dead_zone_max.z)
                target_eye.z = desired_camera_eye.z - camera_dead_zone.z * 0.5f;
            else
                target_eye.z = camera.position.z;

            camera.position = Lerp(camera.position, target_eye, player->camera_follow_speed * dt);
            camera.look_at_position = camera.position + forward(camera.yaw, camera.pitch);
        }
    }
    
    // This can be used to force camera to look at specific position.
    // camera.look_at_position = direction(camera.position, fixed_position);

    //camera.look_at_position = camera.position + direction(camera.position, player->position);
    update_matrices(camera);
    
    // Update player->
    {
        const f32 speed = player->move_speed * dt;
        Vector3 velocity = Vector3_zero;;

        if (input_layer->type == INPUT_LAYER_GAME) {
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

    // @Cleanup @Speed: aabb collision resolve.
    For (manager->entities) {
        auto ea = &it;
        //auto ea = player;
        
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

    // @Speed: slow entity position update.
    For (manager->entities) {
        it.position += it.velocity * dt;
    }
    
    // @Speed: slow entity aabb move and transform update.
    For (manager->entities) {
        move_aabb_along_with_entity(&it);            
    }

    // @Speed: slow entity transform update, final pass.
    For (manager->entities) {
        it.object_to_world = make_transform(it.position, it.orientation, it.scale);
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

    log(LOG_MINIMAL, "Failed to get entity by eid %u (%u %u) from entity manager 0x%X", eid, index, generation, manager);
      
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

// game pak

bool save_game_pak(String path) {
    START_TIMER(0);

    Create_Pak pak;
    String_Builder builder = { .allocator = __temporary_allocator };

    auto &sound_catalog = get_audio_player()->sound_catalog;
    
    {   // Add first entry with game pak metadata.
        put(builder, (u32)GAME_PAK_MAGIC);
        put(builder, (u32)GAME_PAK_VERSION);
        put(builder, (u8 )4);

        put(builder, (u8 )ASSET_SHADER);
        put(builder, (u16)shader_platform.shader_file_table.count);

        put(builder, (u8 )ASSET_TEXTURE);
        put(builder, (u16)texture_catalog.entries.count);

        put(builder, (u8 )ASSET_SOUND);
        put(builder, (u16)sound_catalog.entries.count);

        put(builder, (u8 )ASSET_FLIP_BOOK);
        put(builder, (u16)flip_book_catalog.entries.count);

        auto s = builder_to_string(builder);
        add(pak, GAME_PAK_META_ENTRY_NAME, make_buffer(s));
    }
    
    For (shader_platform.shader_file_table) {
        const auto buffer = read_file(it.value.path, __temporary_allocator);
        add(pak, it.value.path, buffer);
    }

    For (texture_catalog.entries) {
        const auto buffer = read_file(it.path, __temporary_allocator);
        add(pak, it.path, buffer);
    }

    For (sound_catalog.entries) {
        const auto buffer = read_file(it.path, __temporary_allocator);
        add(pak, it.path, buffer);
    }

    For (flip_book_catalog.entries) {
        const auto buffer = read_file(it.path, __temporary_allocator);
        add(pak, it.path, buffer);
    }
    
    write(pak, path);
    log("Saved %S in %.2fms", path, CHECK_TIMER_MS(0));
    
    return true;
}

bool load_game_pak(String path) {
    START_TIMER(0);

    Load_Pak pak;
    
    if (!load(pak, path)) {
        log(LOG_MINIMAL, "Failed to load %S", path);
        return false;
    }

    const auto &meta_entry = pak.entries[0];
    if (meta_entry.name != GAME_PAK_META_ENTRY_NAME) {
        log(LOG_MINIMAL, "First entry is not meta entry in game pak %S, it's name is %S", path, meta_entry.name);
        return false;
    }

    void *meta_data = meta_entry.buffer.data;

    const u32 magic = *Eat(&meta_data, u32);
    if (magic != GAME_PAK_MAGIC) {
        log(LOG_MINIMAL, "Invalid magic %u in game pak %S, expected %u", magic, path, GAME_PAK_MAGIC);
        return false;
    }

    const u32 version = *Eat(&meta_data, u32);
    if (version != GAME_PAK_VERSION) {
        log(LOG_MINIMAL, "Invalid version %u in game pak %S, expected %u", version, path, GAME_PAK_VERSION);
        return false;
    }

    const u8 pair_count = *Eat(&meta_data, u8);
    if (pair_count == 0) {
        log(LOG_MINIMAL, "Zero pair count in meta entry in game pak %S, it makes no sense", path);
        return false;
    }

    struct Pair { u8 asset_type = 0; u16 asset_count = 0; };
    auto *pairs = New(Pair, pair_count, __temporary_allocator);
    for (u8 i = 0; i < pair_count; ++i) {
        pairs[i].asset_type  = *Eat(&meta_data, u8);
        pairs[i].asset_count = *Eat(&meta_data, u16);
    }
    
    u32 load_count = 1; // 1 as we've already got predefined entry with meta data above
    for (u8 i = 0; i < pair_count; ++i) {
        const auto &pair = pairs[i];

        for (u16 j = 0; j < pair.asset_count; ++j) {
            const auto &entry = pak.entries[load_count + j];

            // @Cleanup: ideally make sort of lookup table for create functions to call,
            // so you can use it something like lut[type](...) instead of switch.
            switch (pair.asset_type) {
            case ASSET_SHADER:    new_shader   (entry.name, make_string(entry.buffer)); break;
            case ASSET_TEXTURE:   new_texture  (entry.name, entry.buffer);              break;
            case ASSET_SOUND:     new_sound    (entry.name, entry.buffer);              break;
            case ASSET_FLIP_BOOK: new_flip_book(entry.name, make_string(entry.buffer)); break;
            }
        }

        load_count += pair.asset_count;
    }
    
    log("Loaded %S in %.2fms", path, CHECK_TIMER_MS(0));
    return true;
}
