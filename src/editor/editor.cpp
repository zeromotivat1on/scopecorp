#include "pch.h"
#include "editor.h"
#include "hot_reload.h"
#include "console.h"
#include "game.h"
#include "entity_manager.h"
#include "cpu_time.h"
#include "file_system.h"
#include "input.h"
#include "window.h"
#include "thread.h"
#include "ui.h"
#include "viewport.h"
#include "render_frame.h"
#include "render_target.h"
#include "triangle_mesh.h"
#include "shader.h"
#include "texture.h"
#include "material.h"
#include "flip_book.h"
#include "vector.h"
#include "matrix.h"
#include "profile.h"
#include "font.h"
#include "input_stack.h"
#include "reflection.h"
#include "collision.h"
#include "stb_image.h"
#include "stb_sprintf.h"
#include <algorithm>
#include <functional>

Level_Set *init_editor_level_set() {
    auto cs = New(Campaign_State);
    auto manager = cs->entity_manager = new_entity_manager();

    // Reserve some reasonable amount of entities.
    array_realloc(manager->entities,           256);
    array_realloc(manager->entities_to_delete, 256);
    array_realloc(manager->free_entities,      256);

    auto set = New(Level_Set);
    set->name = copy_string("editor");
    set->campaign_state = cs;
    
    editor_level_set = set;
    return set;
}

void init_level_editor_hub() {
    auto manager = get_entity_manager();
    
	auto player = New_Entity(manager, E_PLAYER);
    manager->player = player->eid;
    
	{
        auto material = get_material(S("player"));
        auto texture = material->diffuse_texture;

        if (texture) {
            const f32 scale_aspect = (f32)texture->width / texture->height;
            const f32 y_scale = 1.0f * scale_aspect;
            const f32 x_scale = y_scale * scale_aspect;
        
            player->scale = Vector3(x_scale, y_scale, 0.2f);
        }

        player->position = Vector3(0.0f, F32_MIN, 0.0f);
        player->object_to_world = make_transform(player->position, player->orientation, player->scale);
        
        player->aabb.c = player->position + Vector3(0.0f, player->scale.y * 0.5f, 0.0f);
        player->aabb.r = player->scale * 0.5f;

        player->mesh     = S("player");
        player->material = S("player");

        player->move_speed     = 500.0f;
        player->move_direction = SOUTH;
        player->move_sound     = S("player_steps");

        player->camera_offset = Vector3(0.0f, 1.0f, -3.0f);
	}

	{
        auto ground = New_Entity(manager, E_STATIC_MESH);
        
		ground->scale = Vector3(16.0f, 16.0f, 0.0f);
        ground->orientation = make_quaternion_from_axis_angle(Vector3_right, 90.0f);
        ground->uv_scale = Vector2(ground->scale.x, ground->scale.y);
        ground->position = Vector3_zero;
        ground->object_to_world = make_transform(ground->position, ground->orientation, ground->scale);

        auto &aabb = ground->aabb;
        aabb.c = ground->position;
		aabb.r = Vector3(ground->scale.x, 0.0f, ground->scale.y);

        ground->mesh     = S("quad");
        ground->material = S("ground");
	}

	{
        auto cube = New_Entity(manager, E_STATIC_MESH);

		cube->position = Vector3(3.0f, 0.5f, 4.0f);
        cube->object_to_world = make_transform(cube->position, cube->orientation, cube->scale);
        
        auto &aabb = cube->aabb;
        aabb.c = cube->position;
		aabb.r = cube->scale * 0.5f;
        
        cube->mesh     = S("cube");
        cube->material = S("cube");
	}

	{
        auto skybox = New_Entity(manager, E_SKYBOX);
        manager->skybox = skybox->eid;

        skybox->position = Vector3(0.0f, -2.0f, 0.0f);
        skybox->uv_scale = Vector2(8.0f, 4.0f);
        
        skybox->mesh     = S("skybox");
        skybox->material = S("skybox");
	}
    
	{
        auto model = New_Entity(manager, E_STATIC_MESH);
		model->position = Vector3(-2.0f, 0.0f, 2.0f);
        model->orientation = make_quaternion_from_axis_angle(Vector3_right, -90.0f);
        
        auto &aabb = model->aabb;
        aabb.c = model->position;
		aabb.r = model->scale * 0.5f;

        model->mesh     = S("tower");
        model->material = S("tower");
	}

    {
        auto emitter = New_Entity(manager, E_SOUND_EMITTER);
        emitter->sound_play_spatial = false;
        emitter->scale = Vector3(0.1f);
        emitter->position = Vector3(0.0f, 1.0f, 0.0f);
        emitter->sound = S("wind_ambience");
        set_sound_looping(emitter->sound, true);

        auto &aabb = emitter->aabb;
        aabb.c = emitter->position;
		aabb.r = emitter->scale * 0.5f;
    }
    
    {
        auto light = New_Entity(manager, E_DIRECT_LIGHT);

        light->position = Vector3(0.0f, 5.0f, 0.0f);
        light->orientation = make_quaternion_from_axis_angle(Vector3_right, 0.0f);
        light->scale = Vector3(0.1f);
        
        light->ambient_factor  = Vector3(0.32f);
        light->diffuse_factor  = Vector3_black;
        light->specular_factor = Vector3_black;
        
        auto &aabb = light->aabb;
        aabb.c = light->position;
		aabb.r = light->scale * 0.5f;
    }
    
    {
        auto light = New_Entity(manager, E_POINT_LIGHT);
        
        light->position = Vector3(0.0f, 2.0f, 0.0f);
        light->scale = Vector3(0.1f);
        
        light->ambient_factor  = Vector3(0.1f);
        light->diffuse_factor  = Vector3(0.5f);
        light->specular_factor = Vector3(1.0f);

        light->attenuation_constant  = 1.0f;
        light->attenuation_linear    = 0.09f;
        light->attenuation_quadratic = 0.032f;
                
        auto &aabb = light->aabb;
        aabb.c = light->position;
		aabb.r = light->scale * 0.5f;
    }

    auto window = get_window();
    auto &camera = manager->camera;
	camera.mode = PERSPECTIVE;
	camera.yaw = 90.0f;
	camera.pitch = 0.0f;
	camera.position = player->position + player->camera_offset;
	camera.look_at_position = camera.position + forward(camera.yaw, camera.pitch);
	camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);
	camera.fov = 60.0f;
	camera.near_plane = 0.001f;
	camera.far_plane = 1000.0f;
	camera.left = 0.0f;
	camera.right = (f32)window->width;
	camera.bottom = 0.0f;
	camera.top = (f32)window->height;
}

struct Find_Entity_By_AABB_Data {
    Entity *e  = null;
    AABB *aabb = null;
};

void mouse_pick_entity(Entity *e) {
    if (editor.mouse_picked_entity) {
        editor.mouse_picked_entity->bits &= ~E_MOUSE_PICKED_BIT;
    }
                
    e->bits |= E_MOUSE_PICKED_BIT;
    editor.mouse_picked_entity = e;

    game_state.selected_entity_property_to_change = PROPERTY_LOCATION;
}

void mouse_unpick_entity() {
    if (editor.mouse_picked_entity) {
        editor.mouse_picked_entity->bits &= ~E_MOUSE_PICKED_BIT;
        editor.mouse_picked_entity = null;
    }
}

void on_editor_push() {
    screen_report("Editor");
}

void on_editor_pop() {
    mouse_unpick_entity();
}

void on_editor_input(const Window_Event *e) {
    auto window = get_window();

    const auto ctrl = down(KEY_CTRL);
    const auto alt  = down(KEY_ALT);
    
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
        } else if (press && ctrl && key == KEY_S) {
            //save_level(World);
        } else if (press && ctrl && key == KEY_R) {         
            //auto path = tprint("%S%S", PATH_LEVEL(""), World.name);
            //load_level(World, path);
            
        } else if (press && alt && key == KEY_V) {
            set_vsync(window, !window->vsync);
        }

        if (editor.mouse_picked_entity) {
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
    case WINDOW_EVENT_MOUSE_CLICK: {
        const auto key   = e->key_code;
        const auto press = e->input_bits & WINDOW_EVENT_PRESS_BIT;

        if (press && key == MOUSE_MIDDLE) {
            lock_cursor(window, !window->cursor_locked);
        } else if (press && key == MOUSE_RIGHT && !window->cursor_locked) {
            mouse_unpick_entity();
        } else if (press && key == MOUSE_LEFT && !window->cursor_locked && ui.id_hot == UIID_NONE) {
            // @Cleanup: specify memory barrier target.
            gpu_memory_barrier();

            auto manager = get_entity_manager();
            auto e = get_entity(manager, gpu_picking_data->eid);
            if (e) mouse_pick_entity(e);
        }
        
        break;
    }
    }
}

void update_editor() {
    Profile_Zone(__func__);

    const f32 dt = get_delta_time();
    
    auto window      = get_window();
    auto input       = get_input_table();
    auto input_layer = get_current_input_layer();
    auto manager     = get_entity_manager();
    auto player      = get_player(manager);
    auto &camera     = manager->camera;
    
    const bool ctrl  = down(KEY_CTRL);
    const bool shift = down(KEY_SHIFT);
    
    // Update editor specific stuff.
    if (program_mode == MODE_EDITOR) {
        // Update camera.
        if (input_layer->type == INPUT_LAYER_EDITOR) {
            const f32 mouse_sensitivity = 16.0f;

            if (window->cursor_locked) {   
                camera.yaw   -= input->mouse_offset_x * mouse_sensitivity * dt;
                camera.pitch += input->mouse_offset_y * mouse_sensitivity * dt;
                camera.pitch  = Clamp(camera.pitch, -89.0f, 89.0f);
            }
                    
            const f32 speed = editor.camera_speed * dt;
            const Vector3 camera_forward = forward(camera.yaw, camera.pitch);
            const Vector3 camera_right = normalize(cross(camera.up_vector, camera_forward));

            Vector3 velocity;
            if (down(KEY_D)) velocity += speed * camera_right;
            if (down(KEY_A)) velocity -= speed * camera_right;
            if (down(KEY_W)) velocity += speed * camera_forward;
            if (down(KEY_S)) velocity -= speed * camera_forward;
            if (down(KEY_E)) velocity += speed * camera.up_vector;
            if (down(KEY_Q)) velocity -= speed * camera.up_vector;

            camera.position += truncate(velocity, speed);
            camera.look_at_position = camera.position + camera_forward;
        }

        update_matrices(camera);

        // @Speed: entity aabb move and transform update.
        For (manager->entities) {
            move_aabb_along_with_entity(&it);
            it.object_to_world = make_transform(it.position, it.orientation, it.scale);
        }
    
        // Update mouse picked entity editor.
        if (input_layer->type == INPUT_LAYER_EDITOR && editor.mouse_picked_entity) {
            // auto e = editor.mouse_picked_entity;

            // {   // Draw entity fields.
            //     constexpr f32 MARGIN  = 100.0f;
            //     constexpr f32 PADDING = 16.0f;
            //     constexpr f32 QUAD_Z = 0.0f;
            
            //     const auto &atlas = *global_font_atlases.main_small;
            //     const f32 ascent  = atlas.ascent  * atlas.px_h_scale;
            //     const f32 descent = atlas.descent * atlas.px_h_scale;

            //     const u32 field_count = entity_type_field_counts[(u8)e->entity_type];
            //     const f32 height = (f32)field_count * atlas.line_height;
            
            //     const Vector2 p0 = Vector2(MARGIN, screen_viewport.height - MARGIN);
            //     const Vector2 p1 = Vector2(screen_viewport.width - MARGIN, p0.y - height - 2 * PADDING);
            //     const u32 qc = rgba_pack(0, 0, 0, 200);

            //     ui_quad(p0, p1, qc, QUAD_Z);

            //     // @Todo: correct uiid generation.
            //     uiid id = { 0, (u16)e->entity_id, 0 };
            //     Vector2 pos = Vector2(p0.x + PADDING, p0.y - PADDING - ascent);
            
            //     for (u32 i = 0; i < field_count; ++i) {
            //         const auto &field = get_entity_field(e->entity_type, i);
            //         const f32 field_name_width = (f32)get_line_width_px(atlas, field.name);
            //         const f32 max_width_f32 = UI_INPUT_BUFFER_SIZE_F32 * atlas.space_xadvance;
                
            //         constexpr u32 tcc = rgba_white;
            //         constexpr u32 tch = rgba_white;
            //         constexpr u32 tca = rgba_white;
            //         constexpr u32 qcc = rgba_pack(32, 32, 32, 200);
            //         constexpr u32 qch = rgba_pack(48, 48, 48, 200);
            //         constexpr u32 qca = rgba_pack(64, 64, 64, 200);
            //         constexpr u32 ccc = rgba_white;
            //         constexpr u32 cch = rgba_white;
            //         constexpr u32 cca = rgba_white;

            //         const f32 offset = atlas.space_xadvance * 40;
            //         UI_Input_Style style = {
            //             global_font_atlases.main_small,
            //             QUAD_Z,
            //             Vector2(pos.x + offset, pos.y),
            //             Vector2_zero,
            //             Vector2_zero,
            //             { tcc, tch, tca },
            //             { qcc, qch, qca },
            //             { ccc, cch, cca },
            //         };

            //         ui_text(field.name, pos, rgba_white, QUAD_Z + F32_EPSILON);

            //         switch (field.type) {
            //         case FIELD_S8: {
            //             auto &v = reflect_field_cast<s8>(*e, field);
            //             ui_input_s8(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_S16: {
            //             auto &v = reflect_field_cast<s16>(*e, field);
            //             ui_input_s16(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_S32: {
            //             auto &v = reflect_field_cast<s32>(*e, field);
            //             ui_input_s32(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_S64: {
            //             auto &v = reflect_field_cast<s64>(*e, field);
            //             ui_input_s64(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_U8: {
            //             auto &v = reflect_field_cast<u8>(*e, field);
            //             ui_input_u8(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_U16: {
            //             auto &v = reflect_field_cast<u16>(*e, field);
            //             ui_input_u16(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_U32: {
            //             auto &v = reflect_field_cast<u32>(*e, field);
            //             ui_input_u32(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_U64: {
            //             auto &v = reflect_field_cast<u64>(*e, field);
            //             ui_input_u64(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_F32: {
            //             auto &v = reflect_field_cast<f32>(*e, field);
            //             ui_input_f32(id, &v, style);
            //             id.index += 1;
            //             break;
            //         }
            //         case FIELD_VECTOR2: {
            //             auto &v = reflect_field_cast<Vector2>(*e, field);

            //             ui_input_f32(id, &v.x, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.y, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             break;
            //         }
            //         case FIELD_VECTOR3: {
            //             auto &v = reflect_field_cast<Vector3>(*e, field);
                    
            //             ui_input_f32(id, &v.x, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.y, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.z, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;
                    
            //             break;
            //         }
            //         case FIELD_QUATERNION: {
            //             auto &v = reflect_field_cast<Quaternion>(*e, field);

            //             ui_input_f32(id, &v.x, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.y, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.z, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;

            //             ui_input_f32(id, &v.w, style);
            //             id.index += 1;
            //             style.pos.x += max_width_f32 + atlas.space_xadvance;
                    
            //             break;
            //         }
            //         }

            //         pos.y -= atlas.line_height;
            //     }
            // }
        
            // if (game_state.selected_entity_property_to_change == PROPERTY_ROTATION) {
            //     const f32 rotate_speed = shift ? 0.04f : 0.01f;

            //     if (down(KEY_LEFT)) {
            //         e->rotation *= make_quaternion_from_axis_angle(Vector3_left, rotate_speed);
            //     }

            //     if (down(KEY_RIGHT)) {
            //         e->rotation *= make_quaternion_from_axis_angle(Vector3_right, rotate_speed);
            //     }

            //     if (down(KEY_UP)) {
            //         const Vector3 direction = ctrl ? Vector3_up : Vector3_forward;
            //         e->rotation *= make_quaternion_from_axis_angle(direction, rotate_speed);
            //     }

            //     if (down(KEY_DOWN)) {
            //         const Vector3 direction = ctrl ? Vector3_down : Vector3_back;
            //         e->rotation *= make_quaternion_from_axis_angle(direction, rotate_speed);
            //     }
            // } else if (game_state.selected_entity_property_to_change == PROPERTY_SCALE) {
            //     const f32 scale_speed = shift ? 4.0f : 1.0f;
                
            //     if (down(KEY_LEFT)) {
            //         e->scale += scale_speed * dt * Vector3_left;
            //     }

            //     if (down(KEY_RIGHT)) {
            //         e->scale += scale_speed * dt * Vector3_right;
            //     }

            //     if (down(KEY_UP)) {
            //         const Vector3 direction = ctrl ? Vector3_up : Vector3_forward;
            //         e->scale += scale_speed * dt * direction;
            //     }

            //     if (down(KEY_DOWN)) {
            //         const Vector3 direction = ctrl ? Vector3_down : Vector3_back;
            //         e->scale += scale_speed * dt * direction;
            //     }
            // } else if (game_state.selected_entity_property_to_change == PROPERTY_LOCATION) {
            //     const f32 move_speed = shift ? 4.0f : 1.0f;

            //     if (down(KEY_LEFT)) {
            //         e->position += move_speed * dt * Vector3_left;
            //     }

            //     if (down(KEY_RIGHT)) {
            //         e->position += move_speed * dt * Vector3_right;
            //     }

            //     if (down(KEY_UP)) {
            //         const Vector3 direction = ctrl ? Vector3_up : Vector3_forward;
            //         e->position += move_speed * dt * direction;
            //     }

            //     if (down(KEY_DOWN)) {
            //         const Vector3 direction = ctrl ? Vector3_down : Vector3_back;
            //         e->position += move_speed * dt * direction;
            //     }
            // }
        } else {
            // @Cleanup: more like a temp hack, clear ui hot id if we are not in editor.
            ui.id_hot = UIID_NONE;
        }

        // Create specific entity.
        if (program_mode != MODE_GAME && window->focused && !window->cursor_locked) {
        //     constexpr f32 Z = 0.0f;
        
        //     constexpr String button_text = S("Add");

        //     const uiid button_id = { 0, 100, 0 };
        //     const uiid combo_id  = { 0, 200, 0 };
        
        //     const UI_Button_Style button_style = {
        //         global_font_atlases.main_small,
        //         Z,
        //         Vector2(100.0f, 100.0f),
        //         Vector2(32.0f, 16.0f),
        //         Vector2_zero,
        //         { rgba_black, rgba_pack(0, 0, 0, 200),       rgba_black },
        //         { rgba_white, rgba_pack(255, 255, 255, 200), rgba_white },
        //     };
        
        //     const auto &atlas = *button_style.font_atlas;
        //     const f32 width = (f32)get_line_width_px(atlas, button_text);
    
        //     const UI_Combo_Style combo_style = {
        //         button_style.font_atlas,
        //         Z,
        //         button_style.pos + Vector2(width + 2 * button_style.padding.x + 4.0f, 0.0f),
        //         button_style.padding,
        //         button_style.margin,
        //         button_style.back_color,
        //         button_style.front_color,
        //     };

        //     const auto button_bits = ui_button(button_id, button_text, button_style);

        //     constexpr u32 selection_offset = 2;
        //     constexpr u32 option_count = carray_count(Entity_type_names) - selection_offset;
        //     const String *options = Entity_type_names + selection_offset;

        //     static u32 selected_entity_type = 0;
        //     ui_combo(combo_id, &selected_entity_type, option_count, options, combo_style);
        
        //     if (button_bits & UI_FINISHED_BIT) {
        //         selected_entity_type += selection_offset;
        //         new_entity(manager, (Entity_Type)(selected_entity_type));
        //     }
        }
    }

    // Screen report.
    {
        editor.report_time += dt;

        f32 fade_time = 0.0f;
        if (editor.report_time > EDITOR_REPORT_SHOW_TIME) {
            fade_time = editor.report_time - EDITOR_REPORT_SHOW_TIME;
            
            if (fade_time > EDITOR_REPORT_FADE_TIME) {
                editor.report_time = 0.0f;
                editor.report_string.count = 0;
            }
        }

        if (editor.report_string.count > 0) {
            constexpr f32 Z = UI_MAX_Z;
            
            const auto &atlas = *global_font_atlases.main_medium;
            const f32 width_px = get_line_width_px(atlas, editor.report_string);
            const Vector2 pos = Vector2(screen_viewport.width * 0.5f - width_px * 0.5f,
                                  screen_viewport.height * 0.7f);

            const f32 lerp_alpha = Clamp(fade_time / EDITOR_REPORT_FADE_TIME, 0.0f, 1.0f);
            const u32 alpha = (u32)Lerp(255, 0, lerp_alpha);
            const u32 color = rgba_pack(255, 255, 255, alpha);

            const Vector2 shadow_offset = Vector2(atlas.px_height * 0.1f, -atlas.px_height * 0.1f);
            const u32 shadow_color = rgba_pack(0, 0, 0, alpha);
            
            ui_text_with_shadow(editor.report_string, pos, color, shadow_offset, shadow_color, Z, &atlas);
        }
    }
}

static void cb_queue_for_hot_reload(const File_Callback_Data *data) {    
    Assert(data->user_data);
    auto catalog = (Hot_Reload_Catalog *)data->user_data;

    // @Cleanup @Robustness: we allocate path in temp storage in hot reload thread, this
    // data will be passed to main thread during hot reload check which may lead to bugs.
    auto path  = copy_string(data->path, __temporary_allocator);
    auto entry = find_by_path(&catalog->catalog, path);
    
    if (entry && entry->last_write_time != data->last_write_time) {
        array_add(catalog->paths_to_hot_reload, path);
        entry->last_write_time = data->last_write_time;
    }
}

static u32 proc_hot_reload(void *data) {    
    Assert(data);
    auto catalog = (Hot_Reload_Catalog *)data;

    while (1) {
        reset_temporary_storage();

        if (catalog->paths_to_hot_reload.count == 0) {
            For (catalog->directories) {
                visit_directory(it, cb_queue_for_hot_reload, true, catalog);
            }
        }
        
        if (catalog->paths_to_hot_reload.count > 0) {
            release_semaphore(catalog->semaphore, 1);
        }
    }
}

static Hot_Reload_Catalog *hot_reload = null;

void init_hot_reload() {
    Assert(!hot_reload);
    hot_reload = New(Hot_Reload_Catalog);
    array_realloc(hot_reload->paths_to_hot_reload, 16);
}

Thread start_hot_reload_thread() {
    hot_reload->semaphore = create_semaphore(0, 1);
    return create_thread(proc_hot_reload, 0, hot_reload);
}

void add_hot_reload_directory(String path) {
    array_add(hot_reload->directories, path);
    add_directory_files(&hot_reload->catalog, path);
	log("New hot reload directory %S", path);
}

void update_hot_reload() {
    Profile_Zone(__func__);

    if (hot_reload->paths_to_hot_reload.count == 0) return;

    wait_semaphore(hot_reload->semaphore, WAIT_INFINITE);

    For (hot_reload->paths_to_hot_reload) {
        const auto ext = get_extension(it);

        bool success = true;
        if (is_shader_source_path(it)) {
            // Ensure we have fresh slang session for proper hot reload.
            init_slang_local_session();
            new_shader(it);
        } else if (is_texture_path(it)) {
            new_texture(it);
        } else if (ext == FLIP_BOOK_EXT) {
            new_flip_book(it);
        } else if (ext == MATERIAL_EXT) {
            new_material(it);
        } else {
            success = false;
        }

        if (success) log("Hot reloaded %S", it);
    }
    
    array_clear(hot_reload->paths_to_hot_reload);
}

// console

inline Console *console = null;

Console *get_console () { Assert(console); return console; }

void init_console() {
    Assert(!console);
    console = New(Console);
    
    console->history.data = (char *)alloc(console->MAX_HISTORY_SIZE);
    console->input.data   = (char *)alloc(console->MAX_INPUT_SIZE);
    
    on_console_viewport_resize(screen_viewport.width, screen_viewport.height);
}

void open_console() {
    Assert(!console->opened);
    console->opened = true;
    push_input_layer(input_layers[INPUT_LAYER_CONSOLE]);
}

void close_console() {
    Assert(console->opened);
    console->opened = false;
    console->cursor_blink_dt = 0.0f;
    auto layer = pop_input_layer();
    Assert(layer->type == INPUT_LAYER_CONSOLE);
}

void update_console() {
    if (!console->opened) return;
    
    auto &history = console->history;
    auto &history_height = console->history_height;
    auto &history_y = console->history_y;
    auto &history_min_y = console->history_min_y;
    auto &history_max_width = console->history_max_width;
    auto &input = console->input;
    auto &cursor_blink_dt = console->cursor_blink_dt;
    
    const auto &atlas = *global_font_atlases.main_small;

    cursor_blink_dt += get_delta_time();
        
    const f32 ascent  = atlas.ascent  * atlas.px_h_scale;
    const f32 descent = atlas.descent * atlas.px_h_scale;
    // @Cleanup: probably not ideal solution to get lower-case glyph height.
    const f32 lower_case_height = (atlas.ascent + atlas.descent) * atlas.px_h_scale;

    constexpr f32 QUAD_Z = 0.0f;

    {   // History quad.
        
        const Vector2 p0 = Vector2(console->MARGIN, console->MARGIN);
        const Vector2 p1 = Vector2(screen_viewport.width - console->MARGIN, screen_viewport.height - console->MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, color, QUAD_Z);
    }

    {   // Input quad.
        const Vector2 q0 = Vector2(console->MARGIN, console->MARGIN);
        const Vector2 q1 = Vector2(screen_viewport.width - console->MARGIN,
                             console->MARGIN + lower_case_height + 2 * console->PADDING);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(q0, q1, color, QUAD_Z);
    }

    {   // Input text.
        const Vector2 pos = Vector2(console->MARGIN + console->PADDING, console->MARGIN + console->PADDING);
        const u32 color = rgba_white;
        ui_text(input, pos, color, QUAD_Z + F32_EPSILON, &atlas);
    }

    {   // Cursor quad.
        const f32 width_px = get_line_width_px(atlas, input);
        const Vector2 p0 = Vector2(console->MARGIN + console->PADDING + width_px + 1,
                             console->MARGIN + console->PADDING + descent);
        const Vector2 p1 = Vector2(p0.x + atlas.space_xadvance, p0.y + ascent - descent);
        u32 color = rgba_white;

        if (cursor_blink_dt > console->CURSOR_BLINK_INTERVAL) {
            if (cursor_blink_dt > 2 * console->CURSOR_BLINK_INTERVAL) {
                cursor_blink_dt = 0.0f;
            } else {
                color = 0;
            }
        }

        ui_quad(p0, p1, color, QUAD_Z);
    }
        
    {   // History text.        
        const f32 max_height = screen_viewport.height - 2 * console->MARGIN - 3 * console->PADDING;

        history_height = 0.0f;
        char *start = history.data;
        f32 visible_height = history.count > 0 ? atlas.line_height : 0.0f;
        f32 current_y = history_y;
        u64 draw_count = 0;
        
        for (s32 i = 0; i < history.count; ++i) {
            if (current_y > history_min_y) {
                start += 1;
                visible_height = 0.0f;
            } else {
                draw_count += 1;
                if (draw_count >= console->MAX_HISTORY_SIZE) {
                    draw_count = console->MAX_HISTORY_SIZE;
                    break;
                }
            }
            
            if (history.data[i] == C_NEW_LINE) {
                history_height += atlas.line_height;
                visible_height += atlas.line_height;
                current_y -= atlas.line_height;
                
                if (visible_height > max_height) {
                    break;
                }
            }
        }

        auto p = Vector2(console->MARGIN + console->PADDING, screen_viewport.height - console->MARGIN - console->PADDING - ascent);
        auto c = rgba_white;
        ui_text(make_string(start, draw_count), p, c, QUAD_Z + F32_EPSILON, &atlas);
    }
}

static void scroll_console(s32 delta) {
    auto &history_height = console->history_height;
    auto &history_y = console->history_y;
    auto &history_min_y = console->history_min_y;

    const auto atlas = global_font_atlases.main_small;
    
    history_y -= delta * atlas->line_height;
    history_y = Clamp(history_y, history_min_y, history_min_y + history_height);
}

void add_to_console_history(String s) {
    auto &history = console->history;
    auto &history_max_width = console->history_max_width;

    const auto &atlas = *global_font_atlases.main_small;

    f32 text_width = 0.0f;
    for (u32 i = 0; i < s.count; ++i) { // @Speed
        const char c = s.data[i];

        // @Cleanup: make better history overflow handling.
        if (history.count > console->MAX_HISTORY_SIZE) {
            history.count = 0;
        }
   
        if (text_width < history_max_width) {
            text_width += get_char_width_px(atlas, c);
            if (c == C_NEW_LINE) {
                text_width = 0;
            }
        } else { // wrap lines that do not fit into debug console window
            history.data[history.count] = C_NEW_LINE;
            history.count += 1;

            text_width = 0;
        }

        history.data[history.count] = c;
        history.count += 1;
    }

    history.data[history.count] = '\0';
}

void on_console_input(const Window_Event *e) {
    auto window = get_window();

    switch (e->type) {
    case WINDOW_EVENT_KEYBOARD: {
        const auto key    = e->key_code;
        const auto press  = e->input_bits & WINDOW_EVENT_PRESS_BIT;
        const auto repeat = e->input_bits & WINDOW_EVENT_REPEAT_BIT;

        if (press && key == KEY_OPEN_CONSOLE) {
            close_console();
        } else if ((press || repeat) && key == KEY_UP) {
            scroll_console(1);
        } else if ((press || repeat) && key == KEY_DOWN) {
            scroll_console(-1);            
        }
        
        break;
    }
    case WINDOW_EVENT_TEXT_INPUT: {
        const auto character = e->character;
        
        if (!console->opened) break;

        if (character == C_GRAVE_ACCENT) {
            break;
        }

        auto &history = console->history;
        auto &history_y = console->history_y;
        auto &history_min_y = console->history_min_y;
        auto &input = console->input;
        auto &cursor_blink_dt = console->cursor_blink_dt;

        const auto &atlas = *global_font_atlases.main_small;

        cursor_blink_dt = 0.0f;
    
        if (character == C_NEW_LINE || character == C_CARRIAGE_RETURN) {
            if (input.count > 0) {                
                // @Todo: make better history overflow handling.
                if (history.count + input.count > console->MAX_HISTORY_SIZE) {
                    history.data[0] = '\0';
                    history.count = 0;
                }

                constexpr auto delims = S(" ");
                const auto tokens = split(input, delims);
                
                String_Builder builder;
                if (tokens[0]) {
                    if (tokens[0] == CONSOLE_CMD_CLEAR) {
                        if (!tokens[1]) {
                            history.data[0] = '\0';
                            history.count = 0;
                            history_y = history_min_y;
                        } else {
                            append(builder, "usage: clear\n");
                            add_to_console_history(builder_to_string(builder));
                        }
                    } else if (tokens[0] == CONSOLE_CMD_LEVEL) {
                        if (tokens[1]) {   
                            //auto path = tprint("%S%S", PATH_LEVEL(""), tokens[1]);
                            //load_level(World, path);
                        } else {
                            append(builder, "usage: level name_with_extension\n");
                            add_to_console_history(builder_to_string(builder));
                        }
                    } else {
                        append(builder, CONSOLE_UNKNOWN_CMD_WARNING);
                        append(builder, input);
                        append(builder, '\n');
                        add_to_console_history(builder_to_string(builder));
                    }
                }
                
                input.count = 0;
            }
        
            break;
        }

        if (character == C_BACKSPACE) {
            if (input.count > 0) {
                input.count -= 1;
            }
        }

        if (Is_Printable(character)) {
            if (input.count >= console->MAX_INPUT_SIZE) {
                break;
            }
        
            input.data[input.count] = (char)character;
            input.count += 1;
        }
        
        break;
    }
    case WINDOW_EVENT_MOUSE_WHEEL: {
        const s32 delta = Sign(e->scroll_delta);
        scroll_console(delta);
        break;
    }
    }
}

void on_console_viewport_resize(s16 width, s16 height) {
    auto &history_y = console->history_y;
    auto &history_min_y = console->history_min_y;
    auto &history_max_width = console->history_max_width;

    history_min_y = height - console->MARGIN;
    history_y = history_min_y;

    history_max_width = width - 2 * console->MARGIN - 2 * console->PADDING;
}

void screen_report(const char *cs, ...) {
    static char buffer[256];

    editor.report_string.data = buffer;
    editor.report_time = 0.0f;

    push_va_args(cs) {
        editor.report_string.count = stbsp_vsnprintf(buffer, carray_count(buffer), cs, va_args);
    }
}

// const Reflect_Field &get_entity_field(Entity_Type type, u32 index) {
//     switch (type) {
//     case E_PLAYER:        return REFLECT_FIELD_AT(Player,        index);
//     case E_SKYBOX:        return REFLECT_FIELD_AT(Skybox,        index);
//     case E_STATIC_MESH:   return REFLECT_FIELD_AT(Static_Mesh,   index);
//     case E_DIRECT_LIGHT:  return REFLECT_FIELD_AT(Direct_Light,  index);
//     case E_POINT_LIGHT:   return REFLECT_FIELD_AT(Point_Light,   index);
//     case E_SOUND_EMITTER: return REFLECT_FIELD_AT(Sound_Emitter, index);
//     case E_PORTAL:        return REFLECT_FIELD_AT(Portal,        index);
//     }

//     unreachable_code_path();
//     return REFLECT_FIELD_AT(Entity, index);
// }

// profiler

static Profiler *profiler = null;

Profiler *get_profiler () { Assert(profiler); return profiler; }

void init_profiler() {
    Assert(!profiler);

    profiler = New(Profiler);

    array_realloc(profiler->all_zones,     MAX_PROFILE_ZONES);
    array_realloc(profiler->active_zones,  MAX_PROFILE_ZONES);
    table_realloc(profiler->zone_lookup,   MAX_PROFILE_ZONES);
}

void open_profiler() {
    Assert(!profiler->opened);
    profiler->opened = true;
    push_input_layer(input_layers[INPUT_LAYER_PROFILER]);
}

void close_profiler() {
    Assert(profiler->opened);
    profiler->opened = false;
    auto layer = pop_input_layer();
    Assert(layer->type == INPUT_LAYER_PROFILER);
}

void update_profiler() {
    using Time_Zone = Profiler::Time_Zone;
    
    // Store separate time zones to preserve data in case of pause.
    static auto zones = Array <Time_Zone> {};

    defer {
        array_clear(profiler->all_zones);
        array_clear(profiler->active_zones);
        table_clear(profiler->zone_lookup);
    };

    if (profiler->all_zones.count) {
        array_clear(zones);
        array_realloc(zones, profiler->all_zones.count);
        
        copy(zones.items, profiler->all_zones.items, profiler->all_zones.count * sizeof(zones.items[0]));
        zones.count = profiler->all_zones.count;
    }

    if (!profiler->opened) return;

    // Common draw data.
    
    constexpr f32 MARGIN  = 100.0f;
    constexpr f32 PADDING = 16.0f;
    constexpr f32 QUAD_Z = 0.0f;

    constexpr u32 MAX_NAME_LENGTH = 24;

    const auto atlas  = global_font_atlases.main_small;
    const auto ascent  = atlas->ascent  * atlas->px_h_scale;
    const auto descent = atlas->descent * atlas->px_h_scale;
    const auto line_height    = atlas->line_height;
    const auto space_width_px = atlas->space_xadvance;

    // Update profiler mode.
    
    switch (profiler->view_mode) {
    case PROFILER_RUNTIME: {
        // Build parent-children relations for correct zone sort and draw.
        struct Children_Per_Zone { u32 count = 0; u32 children[MAX_CHILDREN_PER_ZONE]; };
        Array <u32>               roots    = { .allocator = __temporary_allocator };
        Array <Children_Per_Zone> children = { .allocator = __temporary_allocator };

        array_realloc(children, MAX_PROFILE_ZONES);
        children.count = MAX_PROFILE_ZONES;
        
        u32 zone_index = 0;
        For (zones) {
            if (it.parent == INDEX_NONE) {
                array_add(roots, zone_index);
            } else {
                auto &cpz = children[it.parent];
                cpz.children[cpz.count] = zone_index;
                cpz.count += 1;
            }
        
            zone_index += 1;
        }

        // Prepare draw data.
        
        constexpr String titles[4] = {
            S("Name"), S("Exc"), S("Inc"), S("Num"),
        };

        const f32 max_lengths[carray_count(titles)] = {
            24 * space_width_px,
            10 * space_width_px,
            10 * space_width_px,
            5  * space_width_px,
        };

        f32 offsets[carray_count(titles)] = { MARGIN + PADDING };
        for (auto i = 1; i < carray_count(titles); ++i) {
            offsets[i] = offsets[i - 1] + max_lengths[i - 1];
        }
        
        typedef bool (* Sort_Proc)(u32 a, u32 b);
        constexpr Sort_Proc sort_procs[carray_count(titles)] = {
            [] (u32 a, u32 b) { return (zones[a].name < zones[b].name); },
            [] (u32 a, u32 b) { return (zones[b].exclusive - zones[a].exclusive) < 0; },
            [] (u32 a, u32 b) { return (zones[b].inclusive - zones[a].inclusive) < 0; },
            [] (u32 a, u32 b) { return (zones[b].calls - zones[a].calls) < 0; },
        };
        
        const Sort_Proc sort_proc = sort_procs[profiler->time_sort];

        // Sort roots and children.
        std::stable_sort(roots.items, roots.items + roots.count, sort_proc);

        For (children) {
            std::stable_sort(it.children, it.children + it.count, sort_proc);
        }

        // Build final draw order.
        Array <u32> draw_order = { .allocator = __temporary_allocator };
        const auto dfs = [&] (u32 index) -> void {
            // Nested lambda for outer pretty usage.
            const auto do_dfs = [&] (const auto &self, u32 index) -> void {
                array_add(draw_order, index);

                auto &cpz = children[index];
                for (u32 i = 0; i < cpz.count; ++i) {
                    self(self, cpz.children[i]);
                }
            };

            do_dfs(do_dfs, index);
        };

        For (roots) dfs(it);

        // Start actual draw.
        
        const auto &v = screen_viewport;

        // Background quad.
        const Vector2 p0 = Vector2(MARGIN, v.height - MARGIN - 2 * PADDING - (zones.count + 1.5f) * line_height);
        const Vector2 p1 = Vector2(offsets[carray_count(offsets) - 1] + max_lengths[carray_count(max_lengths) - 1] + PADDING, v.height - MARGIN);
        const u32 c = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, c, QUAD_Z);

        Vector2 p = Vector2(MARGIN + PADDING, v.height - MARGIN - PADDING - ascent);

        // Columns headers.
        for (auto sort = 0; sort < carray_count(titles); ++sort) {
            const f32 z = QUAD_Z + F32_EPSILON;
            const u32 c = profiler->time_sort == sort ? rgba_yellow : rgba_white;
            p.x = offsets[sort];
            ui_text(titles[sort], p, c, z, atlas);
        }

        p.y -= line_height * 1.5f;

        const u64 hz = get_perf_hz_ms();
        
        // Columns data.
        For (draw_order) {
            const auto &zone = zones[it];
            
            const auto name  = zone.name;
            const auto inc   = (f32)zone.inclusive / hz;
            const auto exc   = (f32)zone.exclusive / hz;
            const auto calls = zone.calls;
            const auto depth = zone.depth;
            const auto z     = QUAD_Z + F32_EPSILON;

            const auto t = Clamp(inc / 5.5f, 0.0f, 1.0f);
            const auto r = (u8)(255 * t);
            const auto g = (u8)(255 * (1.0f - t));
            const auto c = rgba_pack(r, g, 0, 255);

            p.x = offsets[0] + depth * space_width_px;
            ui_text(name, p, c, z, atlas);

            p.x = offsets[1];
            ui_text(tprint("%.2f", exc), p, c, z, atlas);

            p.x = offsets[2];
            ui_text(tprint("%.2f", inc), p, c, z, atlas);

            p.x = offsets[3];
            ui_text(tprint("%u", calls), p, c, z, atlas);
            
            p.y -= line_height;
        }
        
        break;
    }
    case PROFILER_MEMORY: {
        // Prepare draw data.
        
        const String titles[3] = {
            S("Name"), S("Used"), S("Size")
        };

        const f32 max_lengths[carray_count(titles)] = {
            24 * space_width_px,
            10 * space_width_px,
            10 * space_width_px,
        };

        f32 offsets[carray_count(titles)] = { MARGIN + PADDING };
        for (auto i = 1; i < carray_count(titles); ++i) {
            offsets[i] = offsets[i - 1] + max_lengths[i - 1];
        }

        extern Virtual_Arena virtual_arena;
        const auto &va  = virtual_arena;
        const auto &ts  = context.temporary_storage;
        const auto &gpu = get_gpu_buffer();
        
        const struct Memory_Zone {
            String name;
            u64 used;
            u64 size;
        };

        Array <Memory_Zone> zones = { .allocator = __temporary_allocator };
        array_add(zones, { S("virtual_arena"),     va.used,      va.reserved    });
        array_add(zones, { S("temporary_storage"), ts->occupied, ts->total_size });
        array_add(zones, { S("gpu_memory"),        gpu->used,    gpu->size      });
        
        // Start actual draw.
        
        const auto &v = screen_viewport;

        // Background quad.
        const Vector2 p0 = Vector2(MARGIN, v.height - MARGIN - 2 * PADDING - (zones.count + 1.5f) * line_height);
        const Vector2 p1 = Vector2(offsets[carray_count(offsets) - 1] + max_lengths[carray_count(max_lengths) - 1] + PADDING, v.height - MARGIN);
        const u32 c = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, c, QUAD_Z);

        Vector2 p = Vector2(MARGIN + PADDING, v.height - MARGIN - PADDING - ascent);

        // Columns headers.
        for (auto sort = 0; sort < carray_count(titles); ++sort) {
            const f32 z = QUAD_Z + F32_EPSILON;
            const u32 c = rgba_white;
            p.x = offsets[sort];
            ui_text(titles[sort], p, c, z, atlas);
        }

        p.y -= line_height * 1.5f;

        // Columns data.
        For (zones) {
            const auto name = it.name;
            const auto used = To_Kilobytes(it.used);
            const auto size = To_Kilobytes(it.size);
            const auto z    = QUAD_Z + F32_EPSILON;

            const auto t = Clamp((f32)used / size, 0.0f, 1.0f);
            const auto r = (u8)(255 * t);
            const auto g = (u8)(255 * (1.0f - t));
            const auto c = rgba_pack(r, g, 0, 255);

            p.x = offsets[0];
            ui_text(name, p, c, z, atlas);

            p.x = offsets[1];
            ui_text(tprint("%llu", used), p, c, z, atlas);

            p.x = offsets[2];
            ui_text(tprint("%llu", size), p, c, z, atlas);
            
            p.y -= line_height;
        }
        
        break;
    }
    default: unreachable_code_path();
    }
}

void switch_profiler_view() {
    profiler->view_mode = (Profiler_View_Mode)((profiler->view_mode + 1) % PROFILER_VIEW_COUNT);
}

void on_profiler_input(const Window_Event *e) {
    switch (e->type) {
    case WINDOW_EVENT_KEYBOARD: {
        const auto key     = e->key_code;
        const auto press   = e->input_bits & WINDOW_EVENT_PRESS_BIT;
        const auto release = e->input_bits & WINDOW_EVENT_RELEASE_BIT;
    
        if (release) break; // do not handle keyboard release events at all

        if (key == KEY_OPEN_PROFILER)        { close_profiler(); break; }
        if (key == KEY_SWITCH_PROFILER_VIEW) { switch_profiler_view(); break; }
        
        switch (profiler->view_mode) {
        case PROFILER_RUNTIME: {
            switch (key) {
            case KEY_P: { profiler->paused = !profiler->paused; break; }
                
            case KEY_1: { profiler->time_sort = 0; break; }
            case KEY_2: { profiler->time_sort = 1; break; }
            case KEY_3: { profiler->time_sort = 2; break; }
            case KEY_4: { profiler->time_sort = 3; break; }
            }
            
            break;
        }
        case PROFILER_MEMORY: {
            // @Todo: no memory profiler specific keys for now.
            break;
        }
        }
        
        break;
    }
    }
}

void push_profile_time_zone(const char *name) { push_profile_time_zone(make_string((char *)name)); }
    
void push_profile_time_zone(String name) {
    if (profiler->paused) return;

    Profiler::Time_Zone *zone = null;
    const auto found = table_find(profiler->zone_lookup, name);
    
    if (!found) {
        zone = &array_add(profiler->all_zones);
        new (zone) Profiler::Time_Zone;
            
        zone->name  = name;
        zone->depth = profiler->active_zones.count;

        table_add(profiler->zone_lookup, name, zone);
    } else {
        zone = *found;
    }

    Assert(zone->depth == profiler->active_zones.count, "For now zones must have unique names across different usage scopes");
    
    zone->calls += 1;
    zone->start  = get_perf_counter();
    
    array_add(profiler->active_zones, zone);
}

void pop_profile_time_zone() {
    if (profiler->paused) return;

    auto zone = array_pop(profiler->active_zones);

    auto time = get_perf_counter() - zone->start;
    auto exc  = time - zone->children;
    
    zone->exclusive += exc;
    zone->inclusive += time;
    
    if (profiler->active_zones.count) {
        auto last = profiler->active_zones[profiler->active_zones.count - 1];
        last->children += time;
    }

    For (profiler->active_zones) {
        if (it->depth + 1 == zone->depth) {
            zone->parent = (s32)(it - profiler->all_zones.items);
            break;
        }
    }
}

// profile

void open(Memory_Profiler &prof) {
    Assert(!prof.is_open);
    prof.is_open = true;
    //push_input_layer(Input_layer_mprof);
}

void close(Memory_Profiler &prof) {
    Assert(prof.is_open);
    prof.is_open = false;
    pop_input_layer();
}

void on_input(Memory_Profiler &prof, const Window_Event &event) {
    auto window = get_window();
        
    switch (event.type) {
    case WINDOW_EVENT_KEYBOARD: {
        const auto key   = event.key_code;
        const auto press = event.input_bits & WINDOW_EVENT_PRESS_BIT;

        if (press && key == KEY_0) {
            prof.sort_col = MEM_PROF_SORT_COL + 0;
        } else if (press && key == KEY_1) {
            prof.sort_col = MEM_PROF_SORT_COL + 1;
        } else if (press && key == KEY_2) {
            prof.sort_col = MEM_PROF_SORT_COL + 2;
        } else if (press && key == KEY_3) {
            prof.sort_col = MEM_PROF_SORT_COL + 3;
        } else if (press && key == KEY_4) {
            prof.sort_col = MEM_PROF_SORT_COL + 4;
        } else if (press && key == KEY_B) {
            prof.precision_type = MEM_PREC_BYTES;
        } else if (press && key == KEY_K) {
            prof.precision_type = MEM_PREC_KB;
        } else if (press && key == KEY_M) {
            prof.precision_type = MEM_PREC_MB;
        } else if (press && key == KEY_G) {
            prof.precision_type = MEM_PREC_GB;
        } else if (press && key == KEY_T) {
            prof.precision_type = MEM_PREC_TB;
        }
        
        break;
    }
    }
}

void draw(Memory_Profiler &prof) {
    /*
    // @Todo: make custom struct that holds allocator ref and name and maybe other metadata.
    static struct {
        Allocator *allocator;
        String name;
    } scopes[3] = {
        
    };

    constexpr u32 scope_count = carray_count(scopes);
        
    constexpr f32 MARGIN  = 100.0f;
    constexpr f32 PADDING = 16.0f;
    constexpr f32 QUAD_Z = 0.0f;

    constexpr u32 NCOL = 4;
    
    if (!prof.is_open) return;
    
    const auto &atlas = *global_font_atlases.main_small;
    const f32 ascent  = atlas.ascent  * atlas.px_h_scale;
    const f32 descent = atlas.descent * atlas.px_h_scale;
    const f32 space_width_px = get_char_width_px(atlas, C_SPACE);
    
    constexpr u32 memory_lengths[MEM_PREC___ca] = { 14, 12, 10, 10, 10 };
    constexpr f64 prec_scales[MEM_PREC___ca] = {
        1.0, 1.0/1024, 1.0/1024/1024, 1.0/1024/1024/1024, 1.0/1024/1024/1024/1024
    };

    const u32 memory_length = memory_lengths[prof.precision_type];
    const f64 prec_scale    = prec_scales[prof.precision_type];

    constexpr String titles  [NCOL] = { S("Name"), S("Used"), S("Commited"), S("Reserved") };
    const u32 max_col_lengths[NCOL] = { 24, memory_length, memory_length, memory_length };

    f32 offsets[NCOL];
    offsets[0] = MARGIN + PADDING;
    offsets[1] = offsets[0] + space_width_px * max_col_lengths[0];
    offsets[2] = offsets[1] + space_width_px * max_col_lengths[1];
    offsets[3] = offsets[2] + space_width_px * max_col_lengths[2];
    
    {   // Profiler quad.
        const Vector2 p0 = Vector2(MARGIN, screen_viewport.height - MARGIN - 2 * PADDING - (scope_count + 3.0f) * atlas.line_height);
        const Vector2 p1 = Vector2(offsets[NCOL - 1] + max_col_lengths[NCOL - 1] * space_width_px + PADDING, screen_viewport.height - MARGIN);
        const u32 color = rgba_pack(0, 0, 0, 200);
        ui_quad(p0, p1, color, QUAD_Z);
    }

    switch (prof.sort_col) {
    case MEM_PROF_SORT_COL + 0: {
        break;
    }
    case MEM_PROF_SORT_COL + 1: {
        std::stable_sort(scopes, scopes + scope_count, [] (const auto &a, const auto &b) {
            return str_compare(a->name, b->name) < 0;
        });
        break;
    }
    case MEM_PROF_SORT_COL + 2: {
        std::stable_sort(scopes, scopes + scope_count, [] (const auto &a, const auto &b) {
            return ((s64)b->allocator->used - (s64)a->allocator->used) < 0;
        });
        break;
    }
    case MEM_PROF_SORT_COL + 3: {
        std::stable_sort(scopes, scopes + scope_count, [] (const auto &a, const auto &b) {
            return ((s64)b->allocator->commited - (s64)a->allocator->commited) < 0;
        });
        break;
    }
    case MEM_PROF_SORT_COL + 4: {
        std::stable_sort(scopes, scopes + scope_count, [] (const auto &a, const auto &b) {
            return ((s64)b->allocator->reserved - (s64)a->allocator->reserved) < 0;
        });
        break;
    }
    }

    u64 total_used = 0;
    u64 total_commited = 0;
    u64 total_reserved = 0;
    
    Vector2 pos = Vector2(MARGIN + PADDING, screen_viewport.height - MARGIN - PADDING - ascent);
    
    for (u32 i = 0; i < NCOL; ++i) {
        u32 color = rgba_white;

        // Index plus one to ignore 0 which means no sort.
        if (i + 1 == prof.sort_col) {
            color = rgba_yellow;
        }
            
        pos.x = offsets[i];
        ui_text(titles[i], pos, color, QUAD_Z + F32_EPSILON, &atlas);
    }

    pos.y -= atlas.line_height * 1.5f;
    
    for (u32 i = 0; i < scope_count; ++i) {
        const auto &name      =  scopes[i].name;
        const auto &allocator = *scopes[i].allocator;
        
        total_used     += allocator.used;
        total_commited += allocator.commited;
        total_reserved += allocator.reserved;
        
        const f32 alpha = Clamp((f32)allocator.used / allocator.reserved, 0.0f, 1.0f);
        const u8 r = (u8)(255 * alpha);
        const u8 g = (u8)(255 * (1.0f - alpha));
        const u32 color = rgba_pack(r, g, 0, 255);
            
        char buffer[256];
        u32 count = 0;

        pos.x = offsets[0];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%S", name);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.x = offsets[1];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", allocator.used * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.x = offsets[2];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", allocator.commited * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);
            
        pos.x = offsets[3];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", allocator.reserved * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.y -= atlas.line_height;
    }

    {   // Total commited/reserved bytes.
        pos.y -= atlas.line_height * 0.5f;

        const f32 alpha = Clamp((f32)total_used / total_reserved, 0.0f, 1.0f);
        const u8 r = (u8)(255 * alpha);
        const u8 g = (u8)(255 * (1.0f - alpha));
        const u32 color = rgba_pack(r, g, 0, 255);
        
        char buffer[256];
        u32 count = 0;

        pos.x = offsets[0];
        count = stbsp_snprintf(buffer, sizeof(buffer), "Total");
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.x = offsets[1];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", total_used * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.x = offsets[2];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", total_commited * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);

        pos.x = offsets[3];
        count = stbsp_snprintf(buffer, sizeof(buffer), "%.2f", total_reserved * prec_scale);
        ui_text(String { buffer, count }, pos, color, QUAD_Z + F32_EPSILON, &atlas);
    }
    */
}

void draw_dev_stats() {
    constexpr f32 Z = UI_MAX_Z;

    auto window  = get_window();
    auto manager = get_entity_manager();
    
    const auto &atlas  = *global_font_atlases.main_small;
	const auto player  = get_player(manager);
	const auto &camera = manager->camera;

	const f32 padding = atlas.px_height * 0.5f;
	const Vector2 shadow_offset = Vector2(atlas.px_height * 0.1f, -atlas.px_height * 0.1f);

    char text[256];
	u32 count = 0;

	Vector2 pos;
    
	{   // Entity.
        pos.x = padding;
		pos.y = (f32)screen_viewport.height - atlas.line_height;
                
        if (editor.mouse_picked_entity) {
            const auto e = editor.mouse_picked_entity;
            const auto property_to_change = game_state.selected_entity_property_to_change;

            const char *change_prop_name = null;
            switch (property_to_change) {
            case PROPERTY_LOCATION: change_prop_name = "position"; break;
            case PROPERTY_ROTATION: change_prop_name = "rotation"; break;
            case PROPERTY_SCALE:    change_prop_name = "scale"; break;
            }
            
            count = stbsp_snprintf(text, sizeof(text),
                                   "%s %u %s", to_string(e->type), e->eid, change_prop_name);
            ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
            pos.y -= 4 * atlas.line_height;
        }
	}

	{   // Stats and states.
        pos.y = (f32)screen_viewport.height - atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text), "%s %s", GAME_VERSION, Build_type_name);
		pos.x = screen_viewport.width - get_line_width_px(atlas, String { text, count }) - padding;
		ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;

        const f32 ms = get_delta_time() * 1000.0f;
		count = stbsp_snprintf(text, sizeof(text), "%.2fms %.ffps", ms, 1000.0f / ms);
        pos.x = screen_viewport.width - get_line_width_px(atlas, String { text, count }) - padding;
        ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;
        
        count = stbsp_snprintf(text, sizeof(text), "window %dx%d",
                               window->width, window->height);
        pos.x = screen_viewport.width - get_line_width_px(atlas, String { text, count }) - padding;
        ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text), "viewport %dx%d",
                               screen_viewport.width, screen_viewport.height);
        pos.x = screen_viewport.width - get_line_width_px(atlas, String { text, count }) - padding;
        ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
        pos.y -= atlas.line_height;

        count = stbsp_snprintf(text, sizeof(text), "draw calls %d", get_draw_call_count());
        pos.x = screen_viewport.width - get_line_width_px(atlas, String { text, count }) - padding;
        ui_text_with_shadow(String { text, count }, pos, rgba_white, shadow_offset, rgba_black, Z);
		pos.y -= atlas.line_height;
	}

    reset_draw_call_count();
}
