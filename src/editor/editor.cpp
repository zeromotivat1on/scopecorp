#include "pch.h"
#include "editor/editor.h"
#include "editor/hot_reload.h"
#include "editor/debug_console.h"

#include "game/game.h"
#include "game/world.h"
#include "game/entity.h"

#include "os/time.h"
#include "os/file.h"
#include "os/input.h"
#include "os/window.h"
#include "os/thread.h"

#include "render/ui.h"
#include "render/r_table.h"
#include "render/r_viewport.h"
#include "render/r_stats.h"
#include "render/r_target.h"
#include "render/r_shader.h"
#include "render/r_texture.h"
#include "render/r_material.h"
#include "render/r_mesh.h"
#include "render/r_flip_book.h"

#include "math/math_basic.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "log.h"
#include "str.h"
#include "profile.h"
#include "font.h"
#include "asset.h"
#include "input_stack.h"
#include "reflection.h"
#include "collision.h"

#include "stb_image.h"
#include "stb_sprintf.h"

Input_Key KEY_CLOSE_WINDOW          = KEY_ESCAPE;
Input_Key KEY_SWITCH_EDITOR_MODE    = KEY_F11;
Input_Key KEY_SWITCH_POLYGON_MODE   = KEY_F1;
Input_Key KEY_SWITCH_COLLISION_VIEW = KEY_F2;
Input_Key KEY_SWITCH_DEBUG_CONSOLE  = KEY_GRAVE_ACCENT;

constexpr u32 MAX_SCREEN_REPORT_SIZE = 256;
constexpr f32 SCREEN_REPORT_SHOW_TIME = 2.0f;
constexpr f32 SCREEN_REPORT_FADE_TIME = 0.5f;
static char screen_report_text[MAX_SCREEN_REPORT_SIZE] = {'\0'};
static f32 screen_report_time = 0.0f;

struct Find_Entity_By_AABB_Data {
    Entity *e = null;
    s32 aabb_index = INDEX_NONE;
};

void mouse_pick_entity(Entity *e) {
    if (Editor.mouse_picked_entity) {
        Editor.mouse_picked_entity->bits &= ~E_MOUSE_PICKED_BIT;
    }
                
    e->bits |= E_MOUSE_PICKED_BIT;
    Editor.mouse_picked_entity = e;

    game_state.selected_entity_property_to_change = PROPERTY_LOCATION;
}

void mouse_unpick_entity() {
    if (Editor.mouse_picked_entity) {
        Editor.mouse_picked_entity->bits &= ~E_MOUSE_PICKED_BIT;
        Editor.mouse_picked_entity = null;
    }
}

static For_Result cb_find_entity_by_aabb(Entity *e, void *user_data) {
    auto *data = (Find_Entity_By_AABB_Data *)user_data;

    if (e->aabb_index == data->aabb_index) {
        data->e = e;
        return BREAK;
    }
    
    return CONTINUE;
};

void on_input_editor(const Window_Event &event) {
    const bool press = event.key_press;
    const bool ctrl = event.with_ctrl;
    const bool alt  = event.with_alt;
    const auto key = event.key_code;
        
    switch (event.type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_close_window(Main_window);
        } else if (press && key == KEY_SWITCH_DEBUG_CONSOLE) {
            open_debug_console();
        } else if (press && key == KEY_SWITCH_RUNTIME_PROFILER) {
            open_runtime_profiler();
        } else if (press && key == KEY_SWITCH_MEMORY_PROFILER) {
            open_memory_profiler();
        } else if (press && key == KEY_SWITCH_EDITOR_MODE) {
            game_state.mode = MODE_GAME;
            os_lock_window_cursor(Main_window, true);
            pop_input_layer();
            editor_report("Game");
            mouse_unpick_entity();
        } else if (press && key == KEY_SWITCH_POLYGON_MODE) {
            if (game_state.polygon_mode == R_FILL) {
                game_state.polygon_mode = R_LINE;
            } else {
                game_state.polygon_mode = R_FILL;
            }
        } else if (press && key == KEY_SWITCH_COLLISION_VIEW) {
            if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                game_state.view_mode_flags &= ~VIEW_MODE_FLAG_COLLISION;
            } else {
                game_state.view_mode_flags |= VIEW_MODE_FLAG_COLLISION;
            }
        } else if (press && ctrl && key == KEY_S) {
            save_level(World);
        } else if (press && ctrl && key == KEY_R) {
            char path[MAX_PATH_LENGTH] = {'\0'};
            str_glue(path, DIR_LEVELS);
            str_glue(path, World.name);
            load_level(World, path);
        } else if (press && alt && key == KEY_V) {
            os_set_window_vsync(Main_window, !Main_window.vsync);
        }

        if (Editor.mouse_picked_entity) {
            if (press && key == KEY_Z) {
                game_state.selected_entity_property_to_change = PROPERTY_LOCATION;
            } else if (press && key == KEY_X) {
                game_state.selected_entity_property_to_change = PROPERTY_ROTATION;
            } else if (press && key == KEY_C) {
                game_state.selected_entity_property_to_change = PROPERTY_SCALE;
            }
        }
            
        break;
    }
    case WINDOW_EVENT_MOUSE: {
        if (press && key == MOUSE_MIDDLE) {
            os_lock_window_cursor(Main_window, !Main_window.cursor_locked);
        } else if (press && key == MOUSE_RIGHT && !Main_window.cursor_locked) {
            mouse_unpick_entity();
        } else if (press && key == MOUSE_LEFT && !Main_window.cursor_locked && R_ui.id_hot == UIID_NONE) {
            if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                const Ray ray = ray_from_mouse(World.ed_camera, R_viewport,
                                               input_table.mouse_x, input_table.mouse_y);
                const s32 aabb_index = find_closest_overlapped_aabb(ray, World.aabbs.items, World.aabbs.count);
                if (aabb_index != INDEX_NONE) {
                    Find_Entity_By_AABB_Data find_data;
                    find_data.e = null;
                    find_data.aabb_index = aabb_index;

                    for_each_entity(World, cb_find_entity_by_aabb, &find_data);

                    if (find_data.e) {
                        mouse_pick_entity(find_data.e);
                    }
                }
            } else {
                const auto &target = R_viewport.render_target;
                const u32 x = (u32)R_viewport.mouse_pos.x;
                const u32 y = (u32)R_viewport.mouse_pos.y;
                const u32 color_attachment = 1;
 
                const s32 pixel = r_read_render_target_pixel(target, color_attachment, x, y);

                auto *e = find_entity_by_eid(World, (eid)pixel);
                if (e) {
                    mouse_pick_entity(e);
                }
            }
        }
        
        break;
    }
    }
}

void tick_editor(f32 dt) {
    PROFILE_SCOPE(__FUNCTION__);

    const bool ctrl  = down(KEY_CTRL);
    const bool shift = down(KEY_SHIFT);
    
    const auto *input_layer = get_current_input_layer();
    const auto &player = World.player;
    
    // Tick camera.
    if (game_state.mode == MODE_GAME) {
        World.ed_camera = World.camera;
    } else if (game_state.mode == MODE_EDITOR) {
        if (input_layer->type == INPUT_LAYER_EDITOR) {
            const f32 mouse_sensitivity = player.mouse_sensitivity;
            auto &camera = World.ed_camera;

            if (Main_window.cursor_locked) {   
                camera.yaw -= input_table.mouse_offset_x * mouse_sensitivity * dt;
                camera.pitch += input_table.mouse_offset_y * mouse_sensitivity * dt;
                camera.pitch = Clamp(camera.pitch, -89.0f, 89.0f);
            }
                    
            const f32 speed = player.ed_camera_speed * dt;
            const vec3 camera_forward = forward(camera.yaw, camera.pitch);
            const vec3 camera_right = camera.up.cross(camera_forward).normalize();

            vec3 velocity;

            if (down(KEY_D))
                velocity += speed * camera_right;

            if (down(KEY_A))
                velocity -= speed * camera_right;

            if (down(KEY_W))
                velocity += speed * camera_forward;

            if (down(KEY_S))
                velocity -= speed * camera_forward;

            if (down(KEY_E))
                velocity += speed * camera.up;

            if (down(KEY_Q))
                velocity -= speed * camera.up;

            camera.eye += velocity.truncate(speed);
            camera.at = camera.eye + camera_forward;
        }
    }
    
    // Tick mouse picked entity editor.
    if (input_layer->type == INPUT_LAYER_EDITOR && Editor.mouse_picked_entity) {
        auto *e = Editor.mouse_picked_entity;

        {   // Draw entity fields.
            constexpr f32 MARGIN  = 100.0f;
            constexpr f32 PADDING = 16.0f;
            constexpr f32 QUAD_Z = 0.0f;
            
            const auto &atlas = R_ui.font_atlases[UI_DEFAULT_FONT_ATLAS_INDEX];
            const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
            const f32 descent = atlas.font->descent * atlas.px_h_scale;

            const u32 field_count = entity_type_field_counts[(u8)e->type];
            const f32 height = (f32)field_count * atlas.line_height;
            
            const vec2 p0 = vec2(MARGIN, R_viewport.height - MARGIN);
            const vec2 p1 = vec2(R_viewport.width - MARGIN, p0.y - height - 2 * PADDING);
            const u32 qc = rgba_pack(0, 0, 0, 200);

            ui_quad(p0, p1, qc, QUAD_Z);

            // @Todo: correct uiid generation.
            uiid id = { 0, (u16)e->eid, 0 };
            vec2 pos = vec2(p0.x + PADDING, p0.y - PADDING - ascent);
            
            for (u32 i = 0; i < field_count; ++i) {
                const auto &field = get_entity_field(e->type, i);
                const f32 field_name_width = (f32)get_line_width_px(atlas, field.name);
                const f32 max_width_f32 = (f32)UI_INPUT_BUFFER_SIZE_F32 * atlas.space_advance_width;
                
                constexpr u32 tcc = rgba_white;
                constexpr u32 tch = rgba_white;
                constexpr u32 tca = rgba_white;
                constexpr u32 qcc = rgba_pack(32, 32, 32, 200);
                constexpr u32 qch = rgba_pack(48, 48, 48, 200);
                constexpr u32 qca = rgba_pack(64, 64, 64, 200);
                constexpr u32 ccc = rgba_white;
                constexpr u32 cch = rgba_white;
                constexpr u32 cca = rgba_white;

                const s32 offset = atlas.space_advance_width * 40;
                UI_Input_Style style = {
                    QUAD_Z,
                    vec2(pos.x + offset, pos.y),
                    vec2_zero,
                    { tcc, tch, tca },
                    { qcc, qch, qca },
                    { ccc, cch, cca },
                };

                ui_text(field.name, pos, rgba_white, QUAD_Z + F32_EPSILON);

                switch (field.type) {
                case FIELD_S8: {
                    auto &v = reflect_field_cast<s8>(*e, field);
                    ui_input_s8(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_S16: {
                    auto &v = reflect_field_cast<s16>(*e, field);
                    ui_input_s16(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_S32: {
                    auto &v = reflect_field_cast<s32>(*e, field);
                    ui_input_s32(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_S64: {
                    auto &v = reflect_field_cast<s64>(*e, field);
                    ui_input_s64(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_U8: {
                    auto &v = reflect_field_cast<u8>(*e, field);
                    ui_input_u8(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_U16: {
                    auto &v = reflect_field_cast<u16>(*e, field);
                    ui_input_u16(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_U32: {
                    auto &v = reflect_field_cast<u32>(*e, field);
                    ui_input_u32(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_U64: {
                    auto &v = reflect_field_cast<u64>(*e, field);
                    ui_input_u64(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_F32: {
                    auto &v = reflect_field_cast<f32>(*e, field);
                    ui_input_f32(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_SID: {
                    auto &v = reflect_field_cast<sid>(*e, field);
                    ui_input_sid(id, &v, style);
                    id.index += 1;
                    break;
                }
                case FIELD_VEC2: {
                    auto &v = reflect_field_cast<vec2>(*e, field);

                    ui_input_f32(id, &v.x, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.y, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    break;
                }
                case FIELD_VEC3: {
                    auto &v = reflect_field_cast<vec3>(*e, field);
                    
                    ui_input_f32(id, &v.x, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.y, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.z, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;
                    
                    break;
                }
                case FIELD_QUAT: {
                    auto &v = reflect_field_cast<quat>(*e, field);

                    ui_input_f32(id, &v.x, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.y, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.z, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;

                    ui_input_f32(id, &v.w, style);
                    id.index += 1;
                    style.pos_text.x += max_width_f32 + atlas.space_advance_width;
                    
                    break;
                }
                }

                pos.y -= atlas.line_height;
            }
        }
        
        if (game_state.selected_entity_property_to_change == PROPERTY_ROTATION) {
            const f32 rotate_speed = shift ? 0.04f : 0.01f;

            if (down(KEY_LEFT)) {
                e->rotation *= quat_from_axis_angle(vec3_left, rotate_speed);
            }

            if (down(KEY_RIGHT)) {
                e->rotation *= quat_from_axis_angle(vec3_right, rotate_speed);
            }

            if (down(KEY_UP)) {
                const vec3 direction = ctrl ? vec3_up : vec3_forward;
                e->rotation *= quat_from_axis_angle(direction, rotate_speed);
            }

            if (down(KEY_DOWN)) {
                const vec3 direction = ctrl ? vec3_down : vec3_back;
                e->rotation *= quat_from_axis_angle(direction, rotate_speed);
            }
        } else if (game_state.selected_entity_property_to_change == PROPERTY_SCALE) {
            const f32 scale_speed = shift ? 4.0f : 1.0f;
                
            if (down(KEY_LEFT)) {
                e->scale += scale_speed * dt * vec3_left;
            }

            if (down(KEY_RIGHT)) {
                e->scale += scale_speed * dt * vec3_right;
            }

            if (down(KEY_UP)) {
                const vec3 direction = ctrl ? vec3_up : vec3_forward;
                e->scale += scale_speed * dt * direction;
            }

            if (down(KEY_DOWN)) {
                const vec3 direction = ctrl ? vec3_down : vec3_back;
                e->scale += scale_speed * dt * direction;
            }
        } else if (game_state.selected_entity_property_to_change == PROPERTY_LOCATION) {
            const f32 move_speed = shift ? 4.0f : 1.0f;

            if (down(KEY_LEFT)) {
                e->location += move_speed * dt * vec3_left;
            }

            if (down(KEY_RIGHT)) {
                e->location += move_speed * dt * vec3_right;
            }

            if (down(KEY_UP)) {
                const vec3 direction = ctrl ? vec3_up : vec3_forward;
                e->location += move_speed * dt * direction;
            }

            if (down(KEY_DOWN)) {
                const vec3 direction = ctrl ? vec3_down : vec3_back;
                e->location += move_speed * dt * direction;
            }
        }
    } else {
        // @Cleanup: more like a temp hack, clear ui hot id if we are not in editor.
        R_ui.id_hot = UIID_NONE;
    }

    // Create specific entity.
    if (game_state.mode == MODE_EDITOR && !Main_window.cursor_locked) {
        constexpr f32 Z = 0.0f;
        
        const char *button_text = "Add";

        const uiid button_id = { 0, 100, 0 };
        const uiid combo_id  = { 0, 200, 0 };
        
        const UI_Button_Style button_style = {
            Z,
            vec2(100.0f, 100.0f),
            vec2(32.0f, 16.0f),
            { rgba_white, rgba_pack(255, 255, 255, 200), rgba_white },
            { rgba_black, rgba_pack(0, 0, 0, 200),       rgba_black },
        };
        
        const auto &atlas = R_ui.font_atlases[button_style.atlas_index];
        const f32 width = (f32)get_line_width_px(atlas, button_text);
    
        const UI_Combo_Style combo_style = {
            Z,
            button_style.pos_text + vec2(width + 2 * button_style.padding.x + 4.0f, 0.0f),
            button_style.padding,
            button_style.color_text,
            button_style.color_quad,
            button_style.atlas_index,
        };

        const auto button_bits = ui_button(button_id, button_text, button_style);

        constexpr u32 selection_offset = 2;
        static u32 selected_entity_type = 0;
        ui_combo(combo_id, &selected_entity_type, entity_type_names + selection_offset, COUNT(entity_type_names) - selection_offset, combo_style);
        
        if (button_bits & UI_FINISHED_BIT) {
            create_entity(World, (Entity_Type)(selected_entity_type + selection_offset));
        }
    }

    {   // Screen report.
        screen_report_time += dt;

        f32 fade_time = 0.0f;
        if (screen_report_time > SCREEN_REPORT_SHOW_TIME) {
            fade_time = screen_report_time - SCREEN_REPORT_SHOW_TIME;
            
            if (fade_time > SCREEN_REPORT_FADE_TIME) {
                screen_report_time = 0.0f;
                screen_report_text[0] = '\0';
            }
        }

        if (screen_report_text[0] != '\0') {
            constexpr f32 Z = UI_MAX_Z;
            const auto atlas_index = UI_SCREEN_REPORT_FONT_ATLAS_INDEX;
            
            const auto &atlas = R_ui.font_atlases[UI_SCREEN_REPORT_FONT_ATLAS_INDEX];
            const s32 width_px = get_line_width_px(atlas, screen_report_text);
            const vec2 pos = vec2(R_viewport.width * 0.5f - width_px * 0.5f,
                                  R_viewport.height * 0.7f);

            const f32 lerp_alpha = Clamp(fade_time / SCREEN_REPORT_FADE_TIME, 0.0f, 1.0f);
            const u32 alpha = (u32)Lerp(255, 0, lerp_alpha);
            const u32 color = rgba_pack(255, 255, 255, alpha);

            const vec2 shadow_offset = vec2(atlas.font_size * 0.1f, -atlas.font_size * 0.1f);
            const u32 shadow_color = rgba_pack(0, 0, 0, alpha);
            
            ui_text_with_shadow(screen_report_text, pos, color, shadow_offset, shadow_color, Z, atlas_index);
        }
    }
}

static void cb_queue_for_hot_reload(const File_Callback_Data *callback_data) {
    char relative_path[MAX_PATH_LENGTH];
    to_relative_asset_path(relative_path, callback_data->path);

    auto &ast = Asset_source_table;
    const auto sid = sid_intern(relative_path);
    
    if (auto *source = find(ast.table, sid)) {
        if (source->last_write_time != callback_data->last_write_time) {
            auto &list = *(Hot_Reload_List *)callback_data->user_data;
            
            Assert(list.reload_count < list.MAX_COUNT);
            list.hot_reload_paths[list.reload_count] = sid;
            list.reload_count += 1;
            
            source->last_write_time = callback_data->last_write_time;
        }
    }
}

static u32 proc_hot_reload(void *data) {
    auto &list = *(Hot_Reload_List *)data;

    while (1) {
        if (list.reload_count == 0) {
            for (s32 i = 0; i < list.path_count; ++i) {
                for_each_file(list.directory_paths[i], &cb_queue_for_hot_reload, &list);
            }
        }
        
        if (list.reload_count > 0) {
            os_release_semaphore(list.semaphore, 1);
        }
    }
}

Thread start_hot_reload_thread(Hot_Reload_List &list) {
    list.semaphore = os_create_semaphore(0, 1);
    return os_create_thread(proc_hot_reload, THREAD_CREATE_IMMEDIATE, &list);
}

void register_hot_reload_directory(Hot_Reload_List &list, const char *path) {
	Assert(list.path_count < list.MAX_DIRS);
    list.directory_paths[list.path_count] = path;
    list.path_count += 1;
	log("Registered hot reload directory %s", path);
}

void check_hot_reload(Hot_Reload_List &list) {
    PROFILE_SCOPE(__FUNCTION__);

    if (list.reload_count == 0) {
        return;
    }

    os_wait_semaphore(list.semaphore, WAIT_INFINITE);

    for (u16 i = 0; i < list.reload_count; ++i) {
        START_SCOPE_TIMER(asset);

        const auto &sid = list.hot_reload_paths[i];
        const auto &asset = *find_asset(sid);

        char path[MAX_PATH_LENGTH];
        to_full_asset_path(path, sid_str(sid));

        const u32 max_data_size = get_asset_max_file_size(asset.type);
        void *data = alloct(max_data_size);
        defer { freet(max_data_size); };

        u64 data_size = 0;
        os_read_file(path, data, max_data_size, &data_size);

        ((char *)data)[data_size] = '\0';

        bool hot_reloaded = true;
        switch (asset.type) {
        case ASSET_SHADER: {
            char *source = (char *)data;
            parse_shader_includes(source);
            
            r_recreate_shader(asset.index, source);
            
            break;
        }
        case ASSET_TEXTURE: {
            const auto &tx = R_table.textures[asset.index];
            
            s32 w, h, cc;
            data = stbi_load_from_memory((u8 *)data, (s32)data_size, &w, &h, &cc, 0);

            const u16 format = r_format_from_channel_count(cc);

            // @Cleanup: some values are default, not really fine.
            r_recreate_texture(asset.index, R_TEXTURE_2D, format, w, h,
                               tx.wrap, tx.min_filter, tx.mag_filter, data);
            
            break;
        }
        /*
        case ASSET_MATERIAL: {
            auto &material = asset_table.materials[hot_reload_asset.sid];
            const char *relative_path = sid_str(material.sid_path);
            convert_to_full_asset_path(path, relative_path);
            
            void *buffer = allocp(MAX_MATERIAL_SIZE);
            defer { freel(MAX_MATERIAL_SIZE); };
            
            u64 bytes_read = 0;
            if (os_file_read(path, buffer, MAX_MATERIAL_SIZE, &bytes_read)) {
                init_material_asset(&material, buffer);
                log("Hot reloaded material %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(asset));
            }
            
            break;
        }
        case ASSET_MESH: {
            auto &mesh = asset_table.meshes[hot_reload_asset.sid];
            const char *relative_path = sid_str(mesh.sid_path);
            convert_to_full_asset_path(path, relative_path);
            
            void *buffer = allocp(MAX_MESH_SIZE);
            defer { freel(MAX_MESH_SIZE); };
            
            u64 bytes_read = 0;
            if (os_file_read(path, buffer, MAX_MATERIAL_SIZE, &bytes_read)) {
                init_mesh_asset(&mesh, buffer);
                log("Hot reloaded mesh %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(asset));
            }
            
            break;
        }
        case ASSET_FLIP_BOOK: {
            auto &flip_book = asset_table.flip_books[hot_reload_asset.sid];
            const char *relative_path = sid_str(flip_book.sid_path);
            convert_to_full_asset_path(path, relative_path);
            
            void *buffer = allocp(MAX_MESH_SIZE);
            defer { freel(MAX_MESH_SIZE); };
            
            u64 bytes_read = 0;
            if (os_file_read(path, buffer, MAX_MATERIAL_SIZE, &bytes_read)) {
                init_flip_book_asset(&flip_book, buffer);
                log("Hot reloaded flip book %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(asset));
            }
            
            break;
        }
        */
        default: {
            error("Unhandled hot reload asset type %d", asset.type);
            hot_reloaded = false;
            break;
        }
        }

        if (hot_reloaded) {
            log("Hot reloaded %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(asset));
        }
    }
    
    list.reload_count = 0;
}

void init_debug_console() {
    Assert(!Debug_console.history);

    auto &history = Debug_console.history;
    auto &history_y = Debug_console.history_y;
    auto &history_min_y = Debug_console.history_min_y;
    auto &history_max_width = Debug_console.history_max_width;

    history = (char *)allocp(MAX_DEBUG_CONSOLE_HISTORY_SIZE);
    history[0] = '\0';

    on_viewport_resize_debug_console(R_viewport.width, R_viewport.height);
}

void open_debug_console() {
    Assert(!Debug_console.is_open);

    Debug_console.is_open = true;

    push_input_layer(Input_layer_debug_console);
}

void close_debug_console() {
    Assert(Debug_console.is_open);
    
    Debug_console.cursor_blink_dt = 0.0f;
    Debug_console.is_open = false;
    
    pop_input_layer();
}

void draw_debug_console() {
    PROFILE_SCOPE(__FUNCTION__);
    
    if (!Debug_console.is_open) return;
    
    auto &history = Debug_console.history;
    auto &history_size = Debug_console.history_size;
    auto &history_height = Debug_console.history_height;
    auto &history_y = Debug_console.history_y;
    auto &history_min_y = Debug_console.history_min_y;
    auto &history_max_width = Debug_console.history_max_width;
    auto &input = Debug_console.input;
    auto &input_size = Debug_console.input_size;
    auto &cursor_blink_dt = Debug_console.cursor_blink_dt;
    
    const auto &atlas = R_ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

    cursor_blink_dt += delta_time;
        
    const f32 ascent  = atlas.font->ascent  * atlas.px_h_scale;
    const f32 descent = atlas.font->descent * atlas.px_h_scale;
    // @Cleanup: probably not ideal solution to get lower-case glyph height.
    const f32 lower_case_height = (atlas.font->ascent + atlas.font->descent) * atlas.px_h_scale;

    constexpr f32 QUAD_Z = 0.0f;
    const auto atlas_index = UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX;

    {   // History quad.
        
        const vec2 p0 = vec2(DEBUG_CONSOLE_MARGIN, DEBUG_CONSOLE_MARGIN);
        const vec2 p1 = vec2(R_viewport.width - DEBUG_CONSOLE_MARGIN,
                             R_viewport.height - DEBUG_CONSOLE_MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, color, QUAD_Z);
    }

    {   // Input quad.
        const vec2 q0 = vec2(DEBUG_CONSOLE_MARGIN, DEBUG_CONSOLE_MARGIN);
        const vec2 q1 = vec2(R_viewport.width - DEBUG_CONSOLE_MARGIN,
                             DEBUG_CONSOLE_MARGIN + lower_case_height + 2 * DEBUG_CONSOLE_PADDING);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(q0, q1, color, QUAD_Z);
    }

    {   // Input text.
        const vec2 pos = vec2(DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING,
                              DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING);
        const u32 color = rgba_white;
        ui_text(input, input_size, pos, color, QUAD_Z + F32_EPSILON, atlas_index);
    }

    {   // History text.
        const vec2 pos = vec2(DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING,
                              R_viewport.height - DEBUG_CONSOLE_MARGIN - DEBUG_CONSOLE_PADDING - ascent);
        const u32 color = rgba_white;
        
        const f32 max_height = R_viewport.height - 2 * DEBUG_CONSOLE_MARGIN - 3 * DEBUG_CONSOLE_PADDING;

        history_height = 0.0f;
        char *start = history;
        f32 visible_height = history_size > 0 ? atlas.line_height : 0.0f;
        f32 current_y = history_y;
        s32 draw_count = 0;
        
        for (s32 i = 0; i < history_size; ++i) {
            if (current_y > history_min_y) {
                start += 1;
                visible_height = 0.0f;
            } else {
                draw_count += 1;
                if (draw_count >= MAX_DEBUG_CONSOLE_HISTORY_SIZE) {
                    draw_count = MAX_DEBUG_CONSOLE_HISTORY_SIZE;
                    break;
                }
            }
            
            if (history[i] == ASCII_NEW_LINE) {
                history_height += atlas.line_height;
                visible_height += atlas.line_height;
                current_y -= atlas.line_height;
                
                if (visible_height > max_height) {
                    break;
                }
            }
        }

        ui_text(start, draw_count, pos, color, QUAD_Z + F32_EPSILON, atlas_index);
    }
    
    {   // Cursor quad.
        const s32 width_px = get_line_width_px(atlas, input, input_size);
        const vec2 p0 = vec2(DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING + width_px + 1,
                             DEBUG_CONSOLE_MARGIN + DEBUG_CONSOLE_PADDING + descent);
        const vec2 p1 = vec2(p0.x + atlas.space_advance_width, p0.y + ascent - descent);
        u32 color = rgba_white;

        if (cursor_blink_dt > DEBUG_CONSOLE_CURSOR_BLINK_INTERVAL) {
            if (cursor_blink_dt > 2 * DEBUG_CONSOLE_CURSOR_BLINK_INTERVAL) {
                cursor_blink_dt = 0.0f;
            } else {
                color = 0;
            }
        }

        ui_quad(p0, p1, color, QUAD_Z + F32_EPSILON);
    }
}

void add_to_debug_console_history(const char *text) {
    add_to_debug_console_history(text, (u32)str_size(text));
}

void add_to_debug_console_history(const char *text, u32 count) {
    if (!Debug_console.history) {
        return;
    }

    auto &history = Debug_console.history;
    auto &history_size = Debug_console.history_size;
    auto &history_max_width = Debug_console.history_max_width;

    const auto &atlas = R_ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

    f32 text_width = 0.0f; 
    for (u32 i = 0; i < count; ++i) {
        const char c = text[i];

        // @Cleanup: make better history overflow handling.
        if (history_size > MAX_DEBUG_CONSOLE_HISTORY_SIZE) {
            history[0] = '\0';
            history_size = 0;
        }
   
        if (text_width < history_max_width) {            
            text_width += get_char_width_px(atlas, c);
            if (c == ASCII_NEW_LINE) {
                text_width = 0;
            }
        } else { // wrap lines that do not fit into debug console window
            history[history_size] = ASCII_NEW_LINE;
            history_size += 1;

            text_width = 0;
        }

        history[history_size] = c;
        history_size += 1; 
    }

    history[history_size] = '\0';
}

static void scroll_debug_console(s32 delta) {
    auto &history_height = Debug_console.history_height;
    auto &history_y = Debug_console.history_y;
    auto &history_min_y = Debug_console.history_min_y;

    const auto &atlas = R_ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];
    
    history_y -= delta * atlas.line_height;
    history_y = Clamp(history_y, history_min_y, history_min_y + history_height);
}

void on_input_debug_console(const Window_Event &event) {
    const bool press = event.key_press;
    const bool repeat = event.key_repeat;
    const auto key = event.key_code;
    const u32 character = event.character;

    switch (event.type) {
    case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            os_close_window(Main_window);
        } else if (press && key == KEY_SWITCH_DEBUG_CONSOLE) {
            close_debug_console();
        } else if ((press || repeat) && key == KEY_UP) {
            scroll_debug_console(1);
        } else if ((press || repeat) && key == KEY_DOWN) {
            scroll_debug_console(-1);            
        }
        
        break;
    }
    case WINDOW_EVENT_TEXT_INPUT: {
        if (!Debug_console.is_open) break;

        if (character == ASCII_GRAVE_ACCENT) {
            break;
        }

        auto &history = Debug_console.history;
        auto &history_size = Debug_console.history_size;
        auto &history_y = Debug_console.history_y;
        auto &history_min_y = Debug_console.history_min_y;
        auto &input = Debug_console.input;
        auto &input_size = Debug_console.input_size;
        auto &cursor_blink_dt = Debug_console.cursor_blink_dt;

        const auto &atlas = R_ui.font_atlases[UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX];

        cursor_blink_dt = 0.0f;
    
        if (character == ASCII_NEW_LINE || character == ASCII_CARRIAGE_RETURN) {
            if (input_size > 0) {
                static char add_text[MAX_DEBUG_CONSOLE_INPUT_SIZE + 128] = { '\0' };
            
                // @Todo: make better history overflow handling.
                if (history_size + input_size > MAX_DEBUG_CONSOLE_HISTORY_SIZE) {
                    history[0] = '\0';
                    history_size = 0;
                }

                const char *DELIMITERS = " ";
                const char *token = str_token(input, DELIMITERS);
                
                if (token) {
                    if (str_cmp(token, DEBUG_CONSOLE_COMMAND_CLEAR)) {
                        const char *next = str_token(null, DELIMITERS);
                        if (!next) {
                            history[0] = '\0';
                            history_size = 0;
                            history_y = history_min_y;
                        } else {
                            str_glue(add_text, "usage: clear\n");
                            add_to_debug_console_history(add_text);
                        }
                    } else if (str_cmp(token, DEBUG_CONSOLE_COMMAND_LEVEL)) {
                        const char *name = str_token(null, DELIMITERS);
                        if (name) {
                            char path[MAX_PATH_LENGTH] = { '\0' };
                            str_glue(path, DIR_LEVELS);
                            str_glue(path, name);
                        
                            load_level(World, path);
                        } else {
                            str_glue(add_text, "usage: level name_with_extension\n");
                            add_to_debug_console_history(add_text);
                        }
                    } else {
                        str_glue(add_text, DEBUG_CONSOLE_UNKNOWN_COMMAND_WARNING);
                        str_glue(add_text, input);
                        str_glue(add_text, "\n");
                        add_to_debug_console_history(add_text);
                    }
   
                }
                
                input_size = 0;
                add_text[0] = '\0';
            }
        
            break;
        }

        if (character == ASCII_BACKSPACE) {
            input_size -= 1;
            input_size = Max(0, input_size);
            input[input_size] = '\0';
        }

        if (is_ascii_printable(character)) {
            if (input_size >= MAX_DEBUG_CONSOLE_INPUT_SIZE) {
                break;
            }
        
            input[input_size] = (char)character;
            input_size += 1;
            input[input_size] = '\0';
        }
        
        break;
    }
    case WINDOW_EVENT_MOUSE: {
        const s32 delta = Sign(event.scroll_delta);
        scroll_debug_console(delta);
        break;
    }
    }
}

void on_viewport_resize_debug_console(s16 width, s16 height) {
    auto &history_y = Debug_console.history_y;
    auto &history_min_y = Debug_console.history_min_y;
    auto &history_max_width = Debug_console.history_max_width;

    history_min_y = height - DEBUG_CONSOLE_MARGIN;
    history_y = history_min_y;

    history_max_width = width - 2 * DEBUG_CONSOLE_MARGIN;
}

void editor_report(const char *str, ...) {
    screen_report_time = 0.0f;

    va_list args;
	va_start(args, str);
    stbsp_vsnprintf(screen_report_text, MAX_SCREEN_REPORT_SIZE, str, args);
	va_end(args);
}

const Reflect_Field &get_entity_field(Entity_Type type, u32 index) {
    switch (type) {
    case E_PLAYER:           return REFLECT_FIELD_AT(Player, index);
    case E_SKYBOX:           return REFLECT_FIELD_AT(Skybox, index);
    case E_STATIC_MESH:      return REFLECT_FIELD_AT(Static_Mesh, index);
    case E_DIRECT_LIGHT:     return REFLECT_FIELD_AT(Direct_Light, index);
    case E_POINT_LIGHT:      return REFLECT_FIELD_AT(Point_Light, index);
    case E_SOUND_EMITTER_2D: return REFLECT_FIELD_AT(Sound_Emitter_2D, index);
    case E_SOUND_EMITTER_3D: return REFLECT_FIELD_AT(Sound_Emitter_3D, index);
    case E_PORTAL:           return REFLECT_FIELD_AT(Portal, index);
    }

    // This should never happen, just make compiler happy.
    return REFLECT_FIELD_AT(Entity, index);
}
