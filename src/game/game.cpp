#include "pch.h"
#include "game/game.h"
#include "game/world.h"

#include "str.h"
#include "log.h"
#include "profile.h"
#include "input_stack.h"
#include "flip_book.h"
#include "asset.h"

#include "math/math_core.h"

#include "os/file.h"
#include "os/input.h"
#include "os/window.h"

#include "editor/editor.h"
#include "editor/debug_console.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"

#include "audio/sound.h"

void on_window_resize(s16 width, s16 height) {
    resize_viewport(&viewport, width, height);
    viewport.orthographic_projection = mat4_orthographic(0, viewport.width, 0, viewport.height, -1, 1);

    const vec2 viewport_resolution = vec2(viewport.width, viewport.height);
    r_set_uniform_block_value(&uniform_block_viewport, 0, 0, &viewport_resolution, get_uniform_type_size_gpu_aligned(UNIFORM_F32_2));        
    r_set_uniform_block_value(&uniform_block_viewport, 1, 0, &viewport.orthographic_projection, get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));        
        
    on_viewport_resize(&world->camera, &viewport);
    world->ed_camera = world->camera;

    on_viewport_resize_debug_console(viewport.width, viewport.height);

    auto &frame_buffer = viewport.frame_buffer;
    auto &material = asset_table.materials[frame_buffer.sid_material];
    const vec2 frame_buffer_resolution = vec2(frame_buffer.width, frame_buffer.height);
    set_material_uniform_value(&material, "u_resolution", &frame_buffer_resolution, sizeof(frame_buffer_resolution));
}

void on_input_game(Window_Event *event) {
    const bool press = event->key_press;
    const bool ctrl = event->with_ctrl;
    const bool alt  = event->with_alt;
    const auto key = event->key_code;
        
    switch (event->type) {
	case WINDOW_EVENT_KEYBOARD: {
        if (press && key == KEY_CLOSE_WINDOW) {
            close(window);
        } else if (press && key == KEY_SWITCH_DEBUG_CONSOLE) {
            open_debug_console();
        } else if (press && key == KEY_SWITCH_PROFILER) {
            open_profiler();
        } else if (press && key == KEY_SWITCH_EDITOR_MODE) {
            game_state.mode = MODE_EDITOR;
            push_input_layer(&input_layer_editor);
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
        } else if (press && ctrl && key == KEY_R) {
            world->player.location = vec3(0.0f, F32_MIN, 0.0f);
            world->camera.eye = world->player.location + world->player.camera_offset;
            world->camera.at = world->camera.eye + forward(world->camera.yaw, world->camera.pitch);
        }

        break;
	}
    }
}

void init_world(World *world) {
	*world = World();

	world->static_meshes  = Sparse_Array<Static_Mesh>(MAX_STATIC_MESHES);
    world->point_lights   = Sparse_Array<Point_Light>(MAX_POINT_LIGHTS);
    world->direct_lights  = Sparse_Array<Direct_Light>(MAX_DIRECT_LIGHTS);
    world->sound_emitters_2d = Sparse_Array<Sound_Emitter_2D>(MAX_SOUND_EMITTERS_2D);
    world->sound_emitters_3d = Sparse_Array<Sound_Emitter_3D>(MAX_SOUND_EMITTERS_3D);
    world->portals        = Sparse_Array<Portal>(MAX_PORTALS);

    world->aabbs = Sparse_Array<AABB>(MAX_AABBS);
}

template <typename T>
static void read_sparse_array(File file, Sparse_Array<T> *array) {
    // @Cleanup: read sparse array directly with replace or add read data to existing one?

    read_file(file, &array->count, sizeof(array->count));

    s32 capacity = 0;
    read_file(file, &capacity, sizeof(capacity));
    Assert(capacity <= array->capacity);

    read_file(file, array->items,  capacity * sizeof(T));
    read_file(file, array->dense,  capacity * sizeof(s32));
    read_file(file, array->sparse, capacity * sizeof(s32));
}

template <typename T>
static void write_sparse_array(File file, Sparse_Array<T> *array) {
    write_file(file, &array->count,    sizeof(array->count));
    write_file(file, &array->capacity, sizeof(array->capacity));
    write_file(file, array->items,    array->capacity * sizeof(T));
    write_file(file, array->dense,    array->capacity * sizeof(s32));
    write_file(file, array->sparse,   array->capacity * sizeof(s32));
}

void save_world_level(World *world) {
    START_SCOPE_TIMER(save);

    char path[MAX_PATH_SIZE];
    str_copy(path, DIR_LEVELS);
    str_glue(path, world->name);
    
    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_WRITE);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        log("World level %s does not exist, creating new one", path);
        file = open_file(path, FILE_OPEN_NEW, FILE_FLAG_WRITE);
        if (file == INVALID_FILE) {
            error("Failed to create new world level %s", path);
            return;
        }
    }

    write_file(file, &world->name, MAX_WORLD_NAME_SIZE);
    
    write_file(file, &world->player,    sizeof(world->player));
    write_file(file, &world->camera,    sizeof(world->camera));
    write_file(file, &world->ed_camera, sizeof(world->ed_camera));
    write_file(file, &world->skybox,    sizeof(world->skybox));

    write_sparse_array(file, &world->static_meshes);
    write_sparse_array(file, &world->point_lights);
    write_sparse_array(file, &world->direct_lights);
    write_sparse_array(file, &world->aabbs);
    
    log("Saved world level %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(save));
}

void load_world_level(World *world, const char *path) {
    START_SCOPE_TIMER(load);

    File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
    defer { close_file(file); };
    
    if (file == INVALID_FILE) {
        error("Failed to open asset pack for load %s", path);
        return;
    }

    read_file(file, &world->name, MAX_WORLD_NAME_SIZE);
    
    read_file(file, &world->player,    sizeof(world->player));
    read_file(file, &world->camera,    sizeof(world->camera));
    read_file(file, &world->ed_camera, sizeof(world->ed_camera));
    read_file(file, &world->skybox,    sizeof(world->skybox));

    read_sparse_array(file, &world->static_meshes);
    read_sparse_array(file, &world->point_lights);
    read_sparse_array(file, &world->direct_lights);
    read_sparse_array(file, &world->aabbs);

    log("Loaded world level %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void tick_game(f32 dt) {
    PROFILE_SCOPE(__FUNCTION__);
    
    const auto *input_layer = get_current_input_layer();
    auto *camera = desired_camera(world);
    auto &player = world->player;
    auto &skybox = world->skybox;
    
	world->dt = dt;

    set_listener_pos(player.location);
    play_sound_or_continue(SID_SOUND_WIND_AMBIENCE, player.location);

    update_matrices(camera);

    r_set_uniform_block_value(&uniform_block_camera, 0, 0, &camera->eye,       get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
    r_set_uniform_block_value(&uniform_block_camera, 1, 0, &camera->view,      get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));
    r_set_uniform_block_value(&uniform_block_camera, 2, 0, &camera->proj,      get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));
    r_set_uniform_block_value(&uniform_block_camera, 3, 0, &camera->view_proj, get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));

    {   // Update skybox.
        auto &aabb = world->aabbs[skybox.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = skybox.location - half_extent;
        aabb.max = skybox.location + half_extent;
        
        auto &material = asset_table.materials[skybox.draw_data.sid_material];
        
        skybox.uv_offset = camera->eye;
        set_material_uniform_value(&material, "u_scale", &skybox.uv_scale, sizeof(skybox.uv_scale));
        set_material_uniform_value(&material, "u_offset", &skybox.uv_offset, sizeof(skybox.uv_offset));
    }

    r_set_uniform_block_value(&uniform_block_direct_lights, 0, 0, &world->direct_lights.count, get_uniform_type_size_gpu_aligned(UNIFORM_U32));
    r_set_uniform_block_value(&uniform_block_point_lights, 0, 0, &world->point_lights.count, get_uniform_type_size_gpu_aligned(UNIFORM_U32));

    For (world->direct_lights) {
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const vec3 light_direction = get_forward_vector(it.rotation);

        // @Speed: its a bit painful to see several set calls instead of just one.
        // @Cleanup: figure out to make it cleaner, maybe get rid of field index parameters;
        // 0 field index is light count, so skipped.
        r_set_uniform_block_value(&uniform_block_direct_lights, 1, it.u_light_index, &light_direction, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 2, it.u_light_index, &it.ambient, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 3, it.u_light_index, &it.diffuse, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 4, it.u_light_index, &it.specular, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
    }
    
    For (world->point_lights) {
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const vec3 light_direction = vec3_zero;
        
        // @Speed: its a bit painful to see several set calls instead of just one.
        // @Cleanup: figure out to make it cleaner, maybe get rid of field index parameters;
        // 0 field index is light count, so skipped.
        r_set_uniform_block_value(&uniform_block_point_lights, 1, it.u_light_index, &it.location, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));

        r_set_uniform_block_value(&uniform_block_point_lights, 2, it.u_light_index, &it.ambient, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        r_set_uniform_block_value(&uniform_block_point_lights, 3, it.u_light_index, &it.diffuse, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        r_set_uniform_block_value(&uniform_block_point_lights, 4, it.u_light_index, &it.specular, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));

        r_set_uniform_block_value(&uniform_block_point_lights, 5, it.u_light_index, &it.attenuation.constant, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
        r_set_uniform_block_value(&uniform_block_point_lights, 6, it.u_light_index, &it.attenuation.linear, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
        r_set_uniform_block_value(&uniform_block_point_lights, 7, it.u_light_index, &it.attenuation.quadratic, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
    }
    
	For (world->static_meshes) {
        // @Todo: take into account rotation and scale.
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        auto &material = asset_table.materials[it.draw_data.sid_material];
        
        const mat4 model = mat4_transform(it.location, it.rotation, it.scale);
		set_material_uniform_value(&material, "u_model", &model, sizeof(model));
        
        set_material_uniform_value(&material, "u_material.ambient",   &material.ambient, sizeof(material.ambient));
        set_material_uniform_value(&material, "u_material.diffuse",   &material.diffuse, sizeof(material.diffuse));
        set_material_uniform_value(&material, "u_material.specular",  &material.specular, sizeof(material.specular));
        set_material_uniform_value(&material, "u_material.shininess", &material.shininess, sizeof(material.shininess));
	}

    // @Todo: fine-tuned sound play.
    
    For (world->sound_emitters_2d) {
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;
        
        play_sound_or_continue(it.sid_sound, get_listener_pos());
    }

    For (world->sound_emitters_3d) {
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;
        
        play_sound_or_continue(it.sid_sound, it.location);
    }
    
    {   // Tick player.
        if (input_layer->type != INPUT_LAYER_GAME) {
            player.velocity = vec3_zero;
        } else {
            const f32 speed = player.move_speed * dt;
            vec3 velocity;

            // @Todo: use input action instead of direct key state.
            if (game_state.player_movement_behavior == MOVE_INDEPENDENT) {
                if (input_table.key_states[KEY_D]) {
                    velocity.x = speed;
                    player.move_direction = DIRECTION_RIGHT;
                }

                if (input_table.key_states[KEY_A]) {
                    velocity.x = -speed;
                    player.move_direction = DIRECTION_LEFT;
                }

                if (input_table.key_states[KEY_W]) {
                    velocity.z = speed;
                    player.move_direction = DIRECTION_FORWARD;
                }

                if (input_table.key_states[KEY_S]) {
                    velocity.z = -speed;
                    player.move_direction = DIRECTION_BACK;
                }
            } else if (game_state.player_movement_behavior == MOVE_RELATIVE_TO_CAMERA) {
                auto &camera = world->camera;
                const vec3 camera_forward = forward(camera.yaw, camera.pitch);
                const vec3 camera_right = camera.up.cross(camera_forward).normalize();

                if (input_table.key_states[KEY_D]) {
                    velocity += speed * camera_right;
                    player.move_direction = DIRECTION_RIGHT;
                }

                if (input_table.key_states[KEY_A]) {
                    velocity -= speed * camera_right;
                    player.move_direction = DIRECTION_LEFT;
                }

                if (input_table.key_states[KEY_W]) {
                    velocity += speed * camera_forward;
                    player.move_direction = DIRECTION_FORWARD;
                }

                if (input_table.key_states[KEY_S]) {
                    velocity -= speed * camera_forward;
                    player.move_direction = DIRECTION_BACK;
                }
            }

            player.velocity = velocity.truncate(speed);
        }

        player.collide_aabb_index = INVALID_INDEX;
        auto &player_aabb = world->aabbs[player.aabb_index];

        // @Temp
        For (world->portals) {
            if (overlap(world->aabbs[it.aabb_index], player_aabb)) {
                player.location = it.destination_location;
            }
        }
        
        for (s32 i = 0; i < world->aabbs.count; ++i) {
            if (i == player.aabb_index) continue;

            // @Temp
            if (i == world->portals[0].aabb_index) continue;
                
            const auto &aabb = world->aabbs[i];
            const vec3 resolved_velocity = resolve_moving_static(player_aabb, aabb, player.velocity);
            if (resolved_velocity != player.velocity) {
                player.velocity = resolved_velocity;
                player.collide_aabb_index = i;
                break;
            }
        }
        
        auto &material = asset_table.materials[player.draw_data.sid_material];
        if (player.velocity == vec3_zero) {
            material.sid_texture = texture_sids.player_idle[player.move_direction];
        } else {
            switch (player.move_direction) {
            case DIRECTION_LEFT: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_LEFT;
                break;
            }
            case DIRECTION_RIGHT: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_RIGHT;
                break;
            }
            case DIRECTION_BACK: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_BACK;
                break;
            }
            case DIRECTION_FORWARD: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_FORWARD;
                break;
            }
            }

            auto &flip_book = asset_table.flip_books[player.sid_flip_book_move];
            tick(&flip_book, dt);

            material.sid_texture = get_current_frame(&flip_book);
        }
            
        player.location += player.velocity;
    
        const auto aabb_offset = vec3(player.scale.x * 0.5f, 0.0f, player.scale.x * 0.3f);
        player_aabb.min = player.location - aabb_offset;
        player_aabb.max = player.location + aabb_offset + vec3(0.0f, player.scale.y, 0.0f);

        if (player.velocity == vec3_zero) {
            stop_sound(player.sid_sound_steps);
        } else {
            play_sound_or_continue(player.sid_sound_steps);
        }

        const mat4 model = mat4_transform(player.location, player.rotation, player.scale);
        set_material_uniform_value(&material, "u_model", &model, sizeof(model));

        set_material_uniform_value(&material, "u_material.ambient",   &material.ambient, sizeof(material.ambient));
        set_material_uniform_value(&material, "u_material.diffuse",   &material.diffuse, sizeof(material.diffuse));
        set_material_uniform_value(&material, "u_material.specular",  &material.specular, sizeof(material.specular));
        set_material_uniform_value(&material, "u_material.shininess", &material.shininess, sizeof(material.shininess));
    }

    {   // Tick camera.
        if (game_state.mode == MODE_GAME) {
            if (game_state.camera_behavior == STICK_TO_PLAYER) {
                auto &camera = world->camera;
                camera.eye = player.location + player.camera_offset;
                camera.at = camera.eye + forward(camera.yaw, camera.pitch);
            } else if (game_state.camera_behavior == FOLLOW_PLAYER) {
                auto &camera = world->camera;
                const vec3 camera_dead_zone = player.camera_dead_zone;
                const vec3 dead_zone_min = camera.eye - camera_dead_zone * 0.5f;
                const vec3 dead_zone_max = camera.eye + camera_dead_zone * 0.5f;
                const vec3 desired_camera_eye = player.location + player.camera_offset;

                vec3 target_eye;
                if (desired_camera_eye.x < dead_zone_min.x)
                    target_eye.x = desired_camera_eye.x + camera_dead_zone.x * 0.5f;
                else if (desired_camera_eye.x > dead_zone_max.x)
                    target_eye.x = desired_camera_eye.x - camera_dead_zone.x * 0.5f;
                else
                    target_eye.x = camera.eye.x;

                if (desired_camera_eye.y < dead_zone_min.y)
                    target_eye.y = desired_camera_eye.y + camera_dead_zone.y * 0.5f;
                else if (desired_camera_eye.y > dead_zone_max.y)
                    target_eye.y = desired_camera_eye.y - camera_dead_zone.y * 0.5f;
                else
                    target_eye.y = camera.eye.y;

                if (desired_camera_eye.z < dead_zone_min.z)
                    target_eye.z = desired_camera_eye.z + camera_dead_zone.z * 0.5f;
                else if (desired_camera_eye.z > dead_zone_max.z)
                    target_eye.z = desired_camera_eye.z - camera_dead_zone.z * 0.5f;
                else
                    target_eye.z = camera.eye.z;

                camera.eye = lerp(camera.eye, target_eye, player.camera_follow_speed * dt);
                camera.at = camera.eye + forward(camera.yaw, camera.pitch);
            }
        }
    }
}

Camera *desired_camera(World *world) {
	if (game_state.mode == MODE_GAME)   return &world->camera;
	if (game_state.mode == MODE_EDITOR) return &world->ed_camera;

	error("Failed to get desired camera from world in unknown game mode %d", game_state.mode);
	return null;
}

Entity *create_entity(World *world, Entity_Type e_type) {
    Entity *e = null;
    switch (e_type) {
    case ENTITY_PLAYER: {
        e = &world->player;
        break;
    }
    case ENTITY_SKYBOX: {
        e = &world->skybox;
        break;
    }
    case ENTITY_STATIC_MESH: {
        e = world->static_meshes.find(world->static_meshes.add_default());
        break;
    }
    case ENTITY_DIRECT_LIGHT: {
        e = world->direct_lights.find(world->direct_lights.add_default());
        break;
    }
    case ENTITY_POINT_LIGHT: {
        e = world->point_lights.find(world->point_lights.add_default());
        break;
    }
    case ENTITY_SOUND_EMITTER_2D: {
        e = world->sound_emitters_2d.find(world->sound_emitters_2d.add_default());
        break;
    }
    case ENTITY_SOUND_EMITTER_3D: {
        e = world->sound_emitters_3d.find(world->sound_emitters_3d.add_default());
        break;
    }
    case ENTITY_PORTAL: {
        e = world->portals.find(world->portals.add_default());
        break;
    }
    }

    if (e) {
        static eid eid = 1;

        e->eid = eid;
        e->aabb_index = world->aabbs.add_default();

        auto &aabb = world->aabbs[e->aabb_index];
        const vec3 half_extent = e->scale * 0.5f;
        aabb.min = e->location - half_extent;
        aabb.max = e->location + half_extent;
        
        eid += 1;
    }
    
    return e;
}

struct Find_Entity_By_Id_Data {
    Entity *e = null;
    eid eid = EID_NONE;
};

static For_Each_Result cb_find_entity_by_eid(Entity *e, void *user_data) {
    auto *data = (Find_Entity_By_Id_Data *)user_data;
    
    if (e->eid == data->eid) {
        data->e = e;
        return RESULT_BREAK;
    }

    return RESULT_CONTINUE;
 };

Entity *find_entity_by_eid(World* world, eid eid) {
    Find_Entity_By_Id_Data find_data;
    find_data.e  = null;
    find_data.eid = eid;
    
    for_each_entity(world, cb_find_entity_by_eid, &find_data);

    if (!find_data.e) {
        warn("Haven't found entity in world by eid %u", eid);
    }
    
    return find_data.e;
}

void for_each_entity(World *world, For_Each_Entity_Callback callback, void *user_data) {
    if (callback(&world->player, user_data) == RESULT_BREAK) return;
    if (callback(&world->skybox, user_data) == RESULT_BREAK) return;

    For (world->static_meshes) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }

    For (world->direct_lights) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }
    
    For (world->point_lights) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }

    For (world->sound_emitters_2d) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }

    For (world->sound_emitters_3d) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }

    For (world->portals) {
        if (callback(&it, user_data) == RESULT_BREAK) return;
    }
}
