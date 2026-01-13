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
            const auto view  = gpu_get_image_view(texture->image_view);
            const auto image = gpu_get_image(view->image);
            
            const f32 scale_aspect = (f32)image->width / image->height;
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
        // @Todo: handle repeat?
        
        const auto key   = e->key_code;
        const auto press = e->press;

        if (press && key == KEY_OPEN_CONSOLE) {
            open_console();
        } else if (press && key == KEY_OPEN_PROFILER) {
            open_profiler();
        } else if (press && key == KEY_SWITCH_POLYGON_MODE) {
            if (game_state.polygon_mode == GPU_POLYGON_FILL) {
                game_state.polygon_mode = GPU_POLYGON_LINE;
            } else {
                game_state.polygon_mode = GPU_POLYGON_FILL;
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
    case WINDOW_EVENT_MOUSE_BUTTON: {
        const auto key   = e->key_code;
        const auto press = e->press;

        if (press && key == MOUSE_RIGHT && !window->cursor_locked) {
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
            const f32 mouse_sensitivity = 32.0f;

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
                editor.report_string.size = 0;
            }
        }

        if (editor.report_string.size > 0) {
            constexpr auto Z = UI_MAX_Z;
            
            const auto atlas    = global_font_atlases.main_medium;
            const auto width_px = get_line_width_px(atlas, editor.report_string);
            const auto pos      = Vector2(screen_viewport.width * 0.5f - width_px * 0.5f,
                                          screen_viewport.height * 0.7f);

            const auto lerp_alpha    = Clamp(fade_time / EDITOR_REPORT_FADE_TIME, 0.0f, 1.0f);
            const auto alpha         = (u32)Lerp(255, 0, lerp_alpha);
            const auto color         = Color32 { .a = alpha, .b = 255, .g = 255, .r = 255 };
            const auto shadow_color  = Color32 { .a = alpha, .b = 0,   .g = 0,   .r = 0   };
            const auto shadow_offset = Vector2(atlas->px_height * 0.1f, -atlas->px_height * 0.1f);
            
            ui_text_with_shadow(editor.report_string, pos, color, shadow_offset, shadow_color, Z, atlas);
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

static Console console;

Console *get_console() { return &console; }

void open_console() {
    Assert(!console.opened);
    console.opened = true;
    push_input_layer(input_layers[INPUT_LAYER_CONSOLE]);
}

void close_console() {
    Assert(console.opened);
    console.opened = false;
    console.cursor_blink_dt = 0.0f;
    auto layer = pop_input_layer();
    Assert(layer->type == INPUT_LAYER_CONSOLE);
}

void update_console() {
    if (!console.opened) return;
    
    auto &history = console.history;
    auto &input   = console.input;
    auto &cursor_blink_dt = console.cursor_blink_dt;
    
    const auto atlas = global_font_atlases.main_small;

    if (!input.data) input.data = (u8 *)alloc(console.MAX_INPUT_SIZE);

    cursor_blink_dt += get_delta_time();
        
    const f32 ascent  = atlas->ascent  * atlas->px_h_scale;
    const f32 descent = atlas->descent * atlas->px_h_scale;
    // @Cleanup: probably not ideal solution to get lower-case glyph height.
    const f32 lower_case_height = (atlas->ascent + atlas->descent) * atlas->px_h_scale;

    constexpr f32 QUAD_Z = 0.0f;

    f32 lowest_message_y = 0;

    {   // History quad.
        
        const auto p0 = Vector2(console.MARGIN, console.MARGIN);
        const auto p1 = Vector2(screen_viewport.width - console.MARGIN, screen_viewport.height - console.MARGIN);
        const auto color = Color32 { .hex = 0x000000AA };
        ui_quad(p0, p1, color, QUAD_Z);
    }
    
    {   // Input quad.
        const auto q0 = Vector2(console.MARGIN, console.MARGIN);
        const auto q1 = Vector2(screen_viewport.width - console.MARGIN,
                                console.MARGIN + lower_case_height + 2 * console.PADDING);
        const auto color = Color32 { .hex = 0x000000AA };
        ui_quad(q0, q1, color, QUAD_Z);

        lowest_message_y += q1.y;
    }

    {   // Input text.
        const auto pos = Vector2(console.MARGIN + console.PADDING, console.MARGIN + console.PADDING);
        const auto color = COLOR32_WHITE;
        ui_text(input, pos, color, QUAD_Z + F32_EPSILON, atlas);
    }

    {   // Cursor quad.
        const auto width_px = get_line_width_px(atlas, input);
        const auto p0 = Vector2(console.MARGIN + console.PADDING + width_px + 1,
                                console.MARGIN + console.PADDING + descent);
        const auto p1 = Vector2(p0.x + atlas->space_xadvance, p0.y + ascent - descent);
        auto color = COLOR32_WHITE;

        if (cursor_blink_dt > console.CURSOR_BLINK_INTERVAL) {
            if (cursor_blink_dt > 2 * console.CURSOR_BLINK_INTERVAL) {
                cursor_blink_dt = 0.0f;
            } else {
                color = {0};
            }
        }

        ui_quad(p0, p1, color, QUAD_Z);
    }
        
    {   // History text.
        constexpr f32 z = QUAD_Z + F32_EPSILON;
        constexpr Color32 color_lut[] = {
            COLOR32_GRAY, COLOR32_WHITE, COLOR32_YELLOW, COLOR32_RED,
        };

        auto p = Vector2(console.MARGIN + console.PADDING, screen_viewport.height - console.MARGIN - console.PADDING - ascent);

        for (s32 i = console.scroll_pos; i < console.message_count; ++i) {
            if (p.y < lowest_message_y) break;
            
            const auto message = console.messages[i];
            const auto level   = console.log_levels[i];
            const auto color   = color_lut[level];

            if (level < context.log_level) continue;
            
            ui_text(message, p, color, z, atlas);
            p.y -= atlas->line_height;
        }        
    }
}

void add_to_console_history(String s) {
    add_to_console_history(LOG_DEFAULT, s);
}

void add_to_console_history(Log_Level level, String s) {
    static const auto add_message = [](Log_Level level, String s) {
        console.log_levels[console.message_count] = level;

        auto &message = console.messages[console.message_count];
        message.data = (u8 *)console.history.data + console.history.size;
        message.size = s.size;
    
        copy(message.data, s.data, s.size);

        console.history.size += s.size;
        console.message_count += 1;
    };
    
    auto &history = console.history;
    if (!history.data) console.history.data = (u8 *)alloc(console.MAX_HISTORY_SIZE);
    
    const auto delims = S("\n");
    const auto tokens = split(s, delims);

    if (history.size + s.size > console.MAX_HISTORY_SIZE ||
        console.message_count + tokens.count > console.MAX_MESSAGE_COUNT) {
        history.size = 0;
        console.message_count = 0;
    }

    For (tokens) {
        const auto pieces = chop(it, 128, __temporary_allocator);
        For (pieces) add_message(level, it);
    }
}

// @Todo: scroll position is messed up after log level change.
static void scroll_console(s32 delta) {
    console.scroll_pos -= delta;
    console.scroll_pos = Clamp(console.scroll_pos, 0, console.message_count);
}

void on_console_input(const Window_Event *e) {
    auto window = get_window();

    switch (e->type) {
    case WINDOW_EVENT_KEYBOARD: {
        // @Todo: handle repeat?
        
        const auto key    = e->key_code;
        const auto press  = e->press;
        const auto repeat = e->repeat;

        if (press && key == KEY_OPEN_CONSOLE) {
            close_console();
        } else if (press && (key == KEY_UP || key == KEY_DOWN)) {
            s32 delta = 1 - 2 * (key == KEY_DOWN);

            if (down(KEY_CTRL))  delta *= console.message_count;
            if (down(KEY_SHIFT)) delta *= 10;

            scroll_console(delta);
        }

        if (down(KEY_ALT)) {
            if (press && key == KEY_1) {
                context.log_level = LOG_VERBOSE;
            } else if (press && key == KEY_2) {
                context.log_level = LOG_DEFAULT;
            } else if (press && key == KEY_3) {
                context.log_level = LOG_WARNING;
            } else if (press && key == KEY_4) {
                context.log_level = LOG_ERROR;
            }
        }
        
        break;
    }
    case WINDOW_EVENT_TEXT_INPUT: {
        const auto character = e->character;
        
        if (!console.opened) break;

        if (character == C_GRAVE_ACCENT) {
            break;
        }

        auto &history = console.history;
        auto &input = console.input;
        auto &cursor_blink_dt = console.cursor_blink_dt;

        const auto &atlas = *global_font_atlases.main_small;

        cursor_blink_dt = 0.0f;
    
        if (character == C_NEW_LINE || character == C_CARRIAGE_RETURN) {
            if (input.size > 0) {
                const auto delims = S(" ");
                const auto tokens = split(input, delims);
                if (!tokens.count) break;
                                
                if (tokens[0]) {
                    if (tokens[0] == CONSOLE_CMD_CLEAR) {
                        if (tokens.count == 1) {
                            history.size = 0;
                            console.message_count = 0;
                        } else {
                            add_to_console_history(CONSOLE_CMD_USAGE_CLEAR);
                        }
                    } else if (tokens[0] == CONSOLE_CMD_LEVEL) {
                        if (tokens.count > 1) {   
                            //auto path = tprint("%S%S", PATH_LEVEL(""), tokens[1]);
                            //load_level(World, path);
                        } else {
                            add_to_console_history(CONSOLE_CMD_USAGE_LEVEL);
                        }
                    } else {
                        String_Builder builder;
                        builder.allocator = __temporary_allocator;
                        
                        append(builder, CONSOLE_CMD_UNKNOWN_WARNING);
                        append(builder, input);
                        
                        add_to_console_history(builder_to_string(builder));
                    }
                }
                
                input.size = 0;
            }
        
            break;
        }

        if (character == C_BACKSPACE) {
            if (input.size > 0) {
                input.size -= 1;
            }
        }

        if (Is_Printable(character)) {
            if (input.size >= console.MAX_INPUT_SIZE) {
                break;
            }
        
            input.data[input.size] = (char)character;
            input.size += 1;
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

void screen_report(const char *cs, ...) {
    static char buffer[256];

    editor.report_string.data = (u8 *)buffer;
    editor.report_time = 0.0f;

    push_va_args(cs) {
        editor.report_string.size = stbsp_vsnprintf(buffer, carray_count(buffer), cs, va_args);
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

static Profiler profiler;

Profiler *get_profiler () { return &profiler; }

void init_profiler() {
    array_realloc(profiler.all_zones,    MAX_PROFILE_ZONES);
    array_realloc(profiler.active_zones, MAX_PROFILE_ZONES);
    table_realloc(profiler.zone_lookup,  MAX_PROFILE_ZONES);
}

void open_profiler() {
    Assert(!profiler.opened);
    profiler.opened = true;
    push_input_layer(input_layers[INPUT_LAYER_PROFILER]);
}

void close_profiler() {
    Assert(profiler.opened);
    profiler.opened = false;
    auto layer = pop_input_layer();
    Assert(layer->type == INPUT_LAYER_PROFILER);
}

void update_profiler() {
    using Time_Zone = Profiler::Time_Zone;
    
    static Array <Time_Zone> zone_arrays[180];
    static u32 display_zones_index = 0;
    static u32 active_zones_index  = 0;
    static u32 zone_arrays_count   = 0;

    defer {
        array_clear(profiler.all_zones);
        array_clear(profiler.active_zones);
        table_clear(profiler.zone_lookup);
    };
    
    auto &zones = zone_arrays[display_zones_index];

    if (!profiler.paused) {
        profiler.update_time += get_delta_time();
        
        if (profiler.update_time >= profiler.UPDATE_INTERVAL) {
            profiler.update_time = 0.0f;

            auto &zones = zone_arrays[active_zones_index];
            array_clear(zones);
            array_realloc(zones, profiler.all_zones.count);
        
            copy(zones.items, profiler.all_zones.items, profiler.all_zones.count * sizeof(zones.items[0]));
            zones.count = profiler.all_zones.count;
     
            active_zones_index += 1;
            display_zones_index = active_zones_index > 1 ? active_zones_index - 2 : 0;
        
            if (active_zones_index >= carray_count(zone_arrays)) {
                active_zones_index  = 0;
                display_zones_index = 0;
            }
        
            zone_arrays_count += 1;
            zone_arrays_count = Min(zone_arrays_count, carray_count(zone_arrays));
        }
    }

    if (!profiler.opened) return;

    // Common draw data.
    
    constexpr f32 MARGIN  = 64.0f;
    constexpr f32 PADDING = 16.0f;
    constexpr f32 QUAD_Z = 0.0f;

    constexpr u32 MAX_NAME_LENGTH = 24;

    const auto atlas  = global_font_atlases.main_small;
    const auto ascent  = atlas->ascent  * atlas->px_h_scale;
    const auto descent = atlas->descent * atlas->px_h_scale;
    const auto line_height    = atlas->line_height;
    const auto space_width_px = atlas->space_xadvance;

    // Update profiler mode.
    
    switch (profiler.view_mode) {
    case PROFILER_VIEW_RUNTIME: {
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
        
        const String titles[4] = {
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
        
        const std::function<bool(u32, u32)> sort_procs[carray_count(titles)] = {
            [&] (u32 a, u32 b) { return (zones[a].name < zones[b].name); },
            [&] (u32 a, u32 b) { return (zones[b].exclusive - zones[a].exclusive) < 0; },
            [&] (u32 a, u32 b) { return (zones[b].inclusive - zones[a].inclusive) < 0; },
            [&] (u32 a, u32 b) { return (zones[b].calls - zones[a].calls) < 0; },
        };
        
        const auto sort_proc = sort_procs[profiler.time_sort];

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
        auto p0 = Vector2(MARGIN, v.height - MARGIN - 2 * PADDING - (zones.count + 1.5f) * line_height); // top left
        auto p1 = Vector2(offsets[carray_count(offsets) - 1] + max_lengths[carray_count(max_lengths) - 1] + PADDING, v.height - MARGIN); // bottom right
        const auto c = Color32 { .hex = 0x000000AA };
        ui_quad(p0, p1, c, QUAD_Z);

        const auto columns_quad_width = Abs(p1.x - p0.x);
        
        auto p = Vector2(MARGIN + PADDING, v.height - MARGIN - PADDING - ascent);

        // Columns headers.
        for (auto sort = 0; sort < carray_count(titles); ++sort) {
            const auto z = QUAD_Z + F32_EPSILON;
            const auto c = profiler.time_sort == sort ? COLOR32_YELLOW : COLOR32_WHITE;
            p.x = offsets[sort];
            ui_text(titles[sort], p, c, z, atlas);
        }

        p.y -= line_height * 1.5f;

        const u64 hz = get_perf_hz_ms();

        auto &selected_zone_index = profiler.selected_zone_index;
        selected_zone_index = Clamp(selected_zone_index, 0, (s32)draw_order.count - 1);
        
        // Rows data.
        auto row_index = 0;
        For (draw_order) {
            if (row_index == selected_zone_index) {
                const auto q0 = Vector2(p0.x, p.y + atlas->line_height);
                const auto q1 = Vector2(p1.x, p.y);
                const auto c  = Color32 { .hex = 0xFFFFFF44 };
                ui_quad(q0, q1, c, QUAD_Z);
            }
            
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
            const auto c = Color32 { .a = 255, .b = 0, .g = g, .r = r };

            p.x = offsets[0] + depth * space_width_px;
            ui_text(name, p, c, z, atlas);

            p.x = offsets[1];
            ui_text(tprint("%.2f", exc), p, c, z, atlas);

            p.x = offsets[2];
            ui_text(tprint("%.2f", inc), p, c, z, atlas);

            p.x = offsets[3];
            ui_text(tprint("%u", calls), p, c, z, atlas);
            
            p.y -= line_height;

            row_index += 1;
        }

        // Draw simple graph of all collected zone arrays.

        const auto zone_width_scale = 4;
        const auto graph_height = 300.0f;
        const auto graph_width  = zone_width_scale * carray_count(zone_arrays) + 2 * PADDING;

        // Background quad.
        p0 = Vector2(MARGIN + columns_quad_width + PADDING, p1.y); // top-left
        p1 = Vector2(p0.x + graph_width, p0.y - graph_height);     // bottom-right
        ui_quad(p0, p1, c, QUAD_Z);

        // Max graph zone time.
        const auto max_zone_time = profiler.max_graph_zone_time;
        const auto text       = tprint("exc scope %.2fms", max_zone_time);
        const auto text_pos   = Vector2(p0.x + PADDING, p1.y + PADDING);
        const auto text_color = COLOR32_WHITE;
        ui_text(text, text_pos, text_color, QUAD_Z + F32_EPSILON, atlas);

        const auto start_of_line_y_offset = 2 * PADDING + atlas->line_height;
        const auto start_of_line_y = p1.y + start_of_line_y_offset;
        const auto max_zone_height = graph_height - start_of_line_y_offset - PADDING;
        
        // Update visualizer.
        const auto lxa = p0.x + PADDING + active_zones_index * zone_width_scale;
        const auto lya = start_of_line_y;
        const auto lxb = lxa;
        const auto lyb = lya + max_zone_height;
        const auto lpa = Vector2(lxa, lya);
        const auto lpb = Vector2(lxb, lyb);
        const auto lc  = COLOR32_WHITE;
        ui_line(lpa, lpb, lc, QUAD_Z + F32_EPSILON);

        const Color32 color_cycle_table[] = {
            COLOR32_RED, COLOR32_GREEN, COLOR32_BLUE,
            COLOR32_YELLOW, COLOR32_PURPLE, COLOR32_CYAN
        };

        // Graph time zone data.
        for (u32 i = 0; i < zone_arrays_count - 1; ++i) {
            const auto &zone_array_a = zone_arrays[i + 0];
            const auto &zone_array_b = zone_arrays[i + 1];
            const auto zone_count    = Min(zone_array_a.count, zone_array_b.count);
            
            for (u32 j = 0; j < zone_count; ++j) {
                const auto &zone_a = zone_array_a[j];
                const auto &zone_b = zone_array_b[j];

                const auto time_a  = (f32)zone_a.exclusive / hz;
                const auto time_b  = (f32)zone_b.exclusive / hz;
                const auto alpha_a = time_a / max_zone_time;
                const auto alpha_b = time_b / max_zone_time;

                auto height_a = Lerp(0.0f, max_zone_height, alpha_a);
                auto height_b = Lerp(0.0f, max_zone_height, alpha_b);

                if (height_a > max_zone_height && height_b > max_zone_height) continue;

                // @Cleanup: these looks a bit meh...
                if (height_a <= max_zone_height && height_a <= height_b)
                    height_b = Min(height_b, max_zone_height);
                if (height_b <= max_zone_height && height_b <= height_a)
                    height_a = Min(height_a, max_zone_height);

                const auto xa = p0.x + PADDING + i * zone_width_scale;
                const auto xb = xa + zone_width_scale;
                const auto ya = start_of_line_y + height_a;
                const auto yb = start_of_line_y + height_b;
                
                const auto pa = Vector2(xa, ya);
                const auto pb = Vector2(xb, yb);

                const auto selected_index = draw_order[selected_zone_index];
                
                const auto ci = j % carray_count(color_cycle_table);
                const auto c  = j == selected_index ? COLOR32_WHITE : color_cycle_table[ci];
                
                ui_line(pa, pb, c, QUAD_Z + F32_EPSILON);
            }
        }
        
        break;
    }
    case PROFILER_VIEW_MEMORY: {
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
        
        const struct Memory_Zone {
            String name;
            u64 used;
            u64 size;
        };

        const auto gpu_read_buffer  = gpu_get_buffer(gpu_read_allocator.buffer);
        const auto gpu_write_buffer = gpu_get_buffer(gpu_write_allocator.buffer);
        
        Array <Memory_Zone> zones = { .allocator = __temporary_allocator };
        array_realloc(zones, 4);
        array_add(zones, { S("virtual_arena"),     va.used, va.reserved });
        array_add(zones, { S("temporary_storage"), ts->total_occupied, ts->total_size });
        array_add(zones, { S("gpu_read_memory"),   gpu_read_allocator.used, gpu_read_buffer->size });
        array_add(zones, { S("gpu_write_memory"),  gpu_write_allocator.used, gpu_write_buffer->size });
        
        // Start actual draw.
        
        const auto &v = screen_viewport;

        // Background quad.
        const auto p0 = Vector2(MARGIN, v.height - MARGIN - 2 * PADDING - (zones.count + 1.5f) * line_height);
        const auto p1 = Vector2(offsets[carray_count(offsets) - 1] + max_lengths[carray_count(max_lengths) - 1] + PADDING, v.height - MARGIN);
        const auto c = Color32 { .hex = 0x000000AA };
        ui_quad(p0, p1, c, QUAD_Z);

        auto p = Vector2(MARGIN + PADDING, v.height - MARGIN - PADDING - ascent);

        // Columns headers.
        for (auto sort = 0; sort < carray_count(titles); ++sort) {
            const auto z = QUAD_Z + F32_EPSILON;
            const auto c = COLOR32_WHITE;
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
            const auto c = Color32 { .a = 255, .b = 0, .g = g, .r = r };

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
    profiler.view_mode = (Profiler_View_Mode)((profiler.view_mode + 1) % PROFILER_VIEW_COUNT);
}

void on_profiler_input(const Window_Event *e) {
    switch (e->type) {
    case WINDOW_EVENT_KEYBOARD: {
        const auto key   = e->key_code;
        const auto press = e->press;
    
        if (!press) break; // do not handle keyboard release events at all

        if (key == KEY_OPEN_PROFILER)        { close_profiler(); break; }
        if (key == KEY_SWITCH_PROFILER_VIEW) { switch_profiler_view(); break; }
        
        switch (profiler.view_mode) {
        case PROFILER_VIEW_RUNTIME: {
            switch (key) {
            case KEY_P: { profiler.paused = !profiler.paused; break; }
                
            case KEY_1: { profiler.time_sort = 0; break; }
            case KEY_2: { profiler.time_sort = 1; break; }
            case KEY_3: { profiler.time_sort = 2; break; }
            case KEY_4: { profiler.time_sort = 3; break; }

            case KEY_UP:
            case KEY_DOWN: {
                auto &index = profiler.selected_zone_index;
                const auto delta = 1 - 2 * (key == KEY_UP);
                index += delta; // updated and used later in update_profiler
                break;
            }
            }
            
            break;
        }
        case PROFILER_VIEW_MEMORY: {
            // @Todo: no memory profiler specific keys for now.
            break;
        }
        }
        
        break;
    }
    case WINDOW_EVENT_MOUSE_WHEEL: {
        switch (profiler.view_mode) {
        case PROFILER_VIEW_RUNTIME: {
            auto delta = Sign(e->scroll_delta);
            if (down(KEY_SHIFT)) delta *= 8;
            if (down(KEY_CTRL))  delta *= 64;
            profiler.max_graph_zone_time -= 0.002f * delta;
            profiler.max_graph_zone_time = Clamp(profiler.max_graph_zone_time, 0.01f, 8.0f);
            break;
        }
        }
        break;
    }
    }
}

void push_profile_time_zone(const char *name) { push_profile_time_zone(make_string((char *)name)); }
    
void push_profile_time_zone(String name) {
    if (profiler.paused) return;

    Profiler::Time_Zone *zone = null;
    const auto found = table_find(profiler.zone_lookup, name);
    
    if (!found) {
        zone = &array_add(profiler.all_zones);
        new (zone) Profiler::Time_Zone;
            
        zone->name  = name;
        zone->depth = profiler.active_zones.count;

        table_add(profiler.zone_lookup, name, zone);
    } else {
        zone = *found;
    }

    Assert(zone->depth == profiler.active_zones.count, "For now zones must have unique names across different usage scopes");
    
    zone->calls += 1;
    zone->start  = get_perf_counter();
    
    array_add(profiler.active_zones, zone);
}

void pop_profile_time_zone() {
    if (profiler.paused) return;

    auto zone = array_pop(profiler.active_zones);

    auto time = get_perf_counter() - zone->start;
    auto exc  = time - zone->children;
    
    zone->exclusive += exc;
    zone->inclusive += time;
    
    if (profiler.active_zones.count) {
        auto last = profiler.active_zones[profiler.active_zones.count - 1];
        last->children += time;
    }

    For (profiler.active_zones) {
        if (it->depth + 1 == zone->depth) {
            zone->parent = (s32)(it - profiler.all_zones.items);
            break;
        }
    }
}

void draw_dev_stats() {
    constexpr f32 Z = UI_MAX_Z;

    auto window  = get_window();
    auto manager = get_entity_manager();
    
    const auto atlas   = global_font_atlases.main_small;
	const auto player  = get_player(manager);
	const auto &camera = manager->camera;

	const auto padding = atlas->px_height * 0.5f;
	const auto shadow_offset = Vector2(atlas->px_height * 0.1f, -atlas->px_height * 0.1f);

    char text[256];
	u32 count = 0;

	Vector2 pos;
    
	{   // Entity.
        pos.x = padding;
		pos.y = (f32)screen_viewport.height - atlas->line_height;
                
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
                                   "%s %u %s", "Entity", e->eid, change_prop_name);
            ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
            pos.y -= 4 * atlas->line_height;
        }
	}

	{   // Stats and states.
        pos.y = (f32)screen_viewport.height - atlas->line_height;

        count = stbsp_snprintf(text, sizeof(text), "%s %S", GAME_VERSION, get_build_type_name());
		pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
		ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
		pos.y -= atlas->line_height;

        const f32 ms = get_delta_time() * 1000.0f;
		count = stbsp_snprintf(text, sizeof(text), "%.2fms %.ffps", ms, 1000.0f / ms);
        pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
        ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
		pos.y -= atlas->line_height;
        
        count = stbsp_snprintf(text, sizeof(text), "window %dx%d",
                               window->width, window->height);
        pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
        ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
		pos.y -= atlas->line_height;

        count = stbsp_snprintf(text, sizeof(text), "viewport %dx%d",
                               screen_viewport.width, screen_viewport.height);
        pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
        ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
        pos.y -= atlas->line_height;

        count = stbsp_snprintf(text, sizeof(text), "draw calls %d", get_draw_call_count());
        pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
        ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
		pos.y -= atlas->line_height;

        const auto input = get_input_table();
        count = stbsp_snprintf(text, sizeof(text), "cursor %d %d", input->mouse_x, input->mouse_y);
        pos.x = screen_viewport.width - get_line_width_px(atlas, make_string(text, count)) - padding;
        ui_text_with_shadow(make_string(text, count), pos, COLOR32_WHITE, shadow_offset, COLOR32_BLACK, Z);
		pos.y -= atlas->line_height;
	}

    reset_draw_call_count();
}
