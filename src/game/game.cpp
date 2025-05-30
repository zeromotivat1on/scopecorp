#include "pch.h"
#include "game/game.h"
#include "game/world.h"

#include "editor/debug_console.h"

#include "str.h"
#include "log.h"
#include "profile.h"
#include "flip_book.h"
#include "asset.h"

#include "math/math_core.h"

#include "os/file.h"
#include "os/input.h"
#include "os/window.h"

#include "render/ui.h"
#include "render/viewport.h"
#include "render/render_registry.h"

#include "audio/sound.h"

static void mouse_pick_entity(World *world, Entity *e) {
    if (world->mouse_picked_entity) {
        world->mouse_picked_entity->flags &= ~ENTITY_FLAG_SELECTED_IN_EDITOR;
    }
                
    e->flags |= ENTITY_FLAG_SELECTED_IN_EDITOR;
    world->mouse_picked_entity = e;

    game_state.selected_entity_property_to_change = PROPERTY_LOCATION;
}

void on_window_event(Window *window, Window_Event *event) {
    switch (event->type) {
	case WINDOW_EVENT_RESIZE: {
        resize_viewport(&viewport, window->width, window->height);
        viewport.orthographic_projection = mat4_orthographic(0, viewport.width, 0, viewport.height, -1, 1);

        const vec2 viewport_resolution = vec2(viewport.width, viewport.height);
        set_uniform_block_value(UNIFORM_BLOCK_VIEWPORT, 0, 0, &viewport_resolution, get_uniform_type_size_gpu_aligned(UNIFORM_F32_2));        
        set_uniform_block_value(UNIFORM_BLOCK_VIEWPORT, 1, 0, &viewport.orthographic_projection, get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));        
        
        on_viewport_resize(&world->camera, &viewport);
        world->ed_camera = world->camera;

        on_debug_console_resize(viewport.width, viewport.height);

        const auto &frame_buffer = render_registry.frame_buffers[viewport.frame_buffer_index];
        const vec2 frame_buffer_resolution = vec2(frame_buffer.width, frame_buffer.height);
        set_material_uniform_value(frame_buffer.material_index, "u_resolution", &frame_buffer_resolution);

        break;
	}
	case WINDOW_EVENT_KEYBOARD: {        
        if (event->key_press && event->key_code == KEY_GRAVE_ACCENT) {
            if (debug_console.is_open) {
                close_debug_console();
            } else {
                open_debug_console();
            }
        }
        
        if (event->key_press && event->key_code == KEY_ESCAPE) {
            close(window);
        }

        // Skip other input process.
        // @Cleanup: create sort of input stack to determine where to pass events.
        if (debug_console.is_open) {
            break;
        }
        
        if (event->key_press && event->with_alt && event->key_code == KEY_G) {
            game_state.mode = MODE_GAME;

            if (world->mouse_picked_entity) {
                world->mouse_picked_entity->flags &= ~ENTITY_FLAG_SELECTED_IN_EDITOR;
                world->mouse_picked_entity = null;
            }
        
            lock_cursor(window, true);
        } else if (event->key_press && event->with_alt && event->key_code == KEY_E) {
            game_state.mode = MODE_EDITOR;
            lock_cursor(window, false);
        } else if (event->key_press && event->with_alt && event->key_code == KEY_C) {
            if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                game_state.view_mode_flags &= ~VIEW_MODE_FLAG_COLLISION;
            } else {
                game_state.view_mode_flags |= VIEW_MODE_FLAG_COLLISION;
            }
        }

        if (game_state.mode == MODE_EDITOR) {
            if (event->key_press && event->key_code == KEY_F1) {
                lock_cursor(window, !window->cursor_locked);
            }

            if (world->mouse_picked_entity) {
                if (event->key_press && event->key_code == KEY_Z) {
                    game_state.selected_entity_property_to_change = PROPERTY_LOCATION;
                } else if (event->key_press && event->key_code == KEY_X) {
                    game_state.selected_entity_property_to_change = PROPERTY_ROTATION;
                } else if (event->key_press && event->key_code == KEY_C) {
                    game_state.selected_entity_property_to_change = PROPERTY_SCALE;
                }
            }
        } else if (game_state.mode == MODE_GAME) {
            if (event->key_press && event->key_code == KEY_F1) {
                game_state.camera_behavior = (Camera_Behavior)(((s32)game_state.camera_behavior + 1) % 3);
            } else if (event->key_press && event->key_code == KEY_F2) {
                game_state.player_movement_behavior = (Player_Movement_Behavior)(((s32)game_state.player_movement_behavior + 1) % 2);
            }
        }
    
        if (event->key_press && event->with_ctrl && event->key_code == KEY_R) {
            world->player.location = vec3(0.0f, F32_MIN, 0.0f);
            world->camera.eye = world->player.location + world->player.camera_offset;
            world->camera.at = world->camera.eye + forward(world->camera.yaw, world->camera.pitch);
        }

        break;
	}
	case WINDOW_EVENT_TEXT_INPUT: {
        if (debug_console.is_open) {
            on_debug_console_input(event->character);
        }
        
        break;
	}
	case WINDOW_EVENT_MOUSE: {
        if (debug_console.is_open) {
            on_debug_console_scroll(Sign(event->scroll_delta));
        }
        else {
            if (game_state.mode == MODE_EDITOR) {
                if (event->key_code == MOUSE_MIDDLE) {
                    lock_cursor(window, !window->cursor_locked);
                }

                if (event->key_press && event->key_code == MOUSE_RIGHT) {
                    if (world->mouse_picked_entity) {
                        world->mouse_picked_entity->flags &= ~ENTITY_FLAG_SELECTED_IN_EDITOR;
                        world->mouse_picked_entity = null;
                    }
                } else if (event->key_press && event->key_code == MOUSE_LEFT) {
                    if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
                        const Ray ray = world_ray_from_viewport_location(desired_camera(world), &viewport, input_table.mouse_x, input_table.mouse_y);
                        const s32 aabb_index = find_closest_overlapped_aabb(ray, world->aabbs.items, world->aabbs.count);
                        if (aabb_index != INVALID_INDEX) {
                            struct Find_Entity_By_AABB_Data {
                                Entity *e = null;
                                s32 aabb_index = INVALID_INDEX;
                            };

                            static const auto cb_find_entity_by_aabb = [] (Entity *e, void *user_data) -> For_Each_Result {
                                auto *data = (Find_Entity_By_AABB_Data *)user_data;

                                switch (e->type) {
                                case ENTITY_PLAYER: {
                                    auto *player = (Player *)e;
                                    if (player->aabb_index == data->aabb_index) {
                                        data->e = e;
                                        return RESULT_BREAK;
                                    }
                                
                                    break;
                                }
                                case ENTITY_STATIC_MESH: {
                                    auto *mesh = (Static_Mesh *)e;
                                    if (mesh->aabb_index == data->aabb_index) {
                                        data->e = e;
                                        return RESULT_BREAK;
                                    }
                                
                                    break;
                                }
                                case ENTITY_POINT_LIGHT: {
                                    auto *light = (Point_Light *)e;
                                    if (light->aabb_index == data->aabb_index) {
                                        data->e = e;
                                        return RESULT_BREAK;
                                    }
                                
                                    break;
                                }
                                case ENTITY_DIRECT_LIGHT: {
                                    auto *light = (Direct_Light *)e;
                                    if (light->aabb_index == data->aabb_index) {
                                        data->e = e;
                                        return RESULT_BREAK;
                                    }
                                
                                    break;
                                }
                                }

                                return RESULT_CONTINUE;
                            };
                        
                            Find_Entity_By_AABB_Data find_data;
                            find_data.e = null;
                            find_data.aabb_index = aabb_index;

                            for_each_entity(world, cb_find_entity_by_aabb, &find_data);

                            if (find_data.e) {
                                mouse_pick_entity(world, find_data.e);
                            }
                        }
                    } else {
                        const s32 id = read_frame_buffer_pixel(viewport.frame_buffer_index, 1, input_table.mouse_x, input_table.mouse_y);
                        auto *e = find_entity_by_id(world, id);
                        if (e) {
                            mouse_pick_entity(world, e);
                        }
                    }
                }
            }
        }
        
        break;
    }
	case WINDOW_EVENT_QUIT: {
		log("WINDOW_EVENT_QUIT");
        break;
	}
    default: {
        error("Unhandled window event %d", event->type);
        break;
    }
    }
}

void init_world(World *world) {
	*world = World();

	world->static_meshes = Sparse_Array<Static_Mesh>(MAX_STATIC_MESHES);
    world->point_lights  = Sparse_Array<Point_Light>(MAX_POINT_LIGHTS);
    world->direct_lights = Sparse_Array<Direct_Light>(MAX_DIRECT_LIGHTS);

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

void save_world(World *world) {
    START_SCOPE_TIMER(save);

    char path[MAX_PATH_SIZE];
    str_copy(path, DIR_LEVELS);
    str_glue(path, world->name);
    str_glue(path, WORLD_LEVEL_EXTENSION_NAME);
    
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

    write_sparse_array(file, &render_registry.vertex_buffers);
    write_sparse_array(file, &render_registry.vertex_arrays);
    write_sparse_array(file, &render_registry.index_buffers);
    write_sparse_array(file, &render_registry.materials);
    write_sparse_array(file, &render_registry.uniforms);
    
    log("Saved world level %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(save));
}

void load_world(World *world, const char *path) {
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

    read_sparse_array(file, &render_registry.vertex_buffers);
    read_sparse_array(file, &render_registry.vertex_arrays);
    read_sparse_array(file, &render_registry.index_buffers);
    read_sparse_array(file, &render_registry.materials);
    read_sparse_array(file, &render_registry.uniforms);

    log("Loaded world level %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
}

void tick(World *world, f32 dt) {
    PROFILE_SCOPE("tick_world");
    
	world->dt = dt;

    play_sound_or_continue(sound_sids.world);
    
	auto *camera = desired_camera(world);
    update_matrices(camera);

    set_uniform_block_value(UNIFORM_BLOCK_CAMERA, 0, 0, &camera->eye,       get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
    set_uniform_block_value(UNIFORM_BLOCK_CAMERA, 1, 0, &camera->view,      get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));
    set_uniform_block_value(UNIFORM_BLOCK_CAMERA, 2, 0, &camera->proj,      get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));
    set_uniform_block_value(UNIFORM_BLOCK_CAMERA, 3, 0, &camera->view_proj, get_uniform_type_size_gpu_aligned(UNIFORM_F32_4X4));
    
	auto &skybox = world->skybox;
	skybox.uv_offset = camera->eye;
	set_material_uniform_value(skybox.draw_data.material_index, "u_scale", &skybox.uv_scale);
	set_material_uniform_value(skybox.draw_data.material_index, "u_offset", &skybox.uv_offset);

    set_uniform_block_value(UNIFORM_BLOCK_DIRECT_LIGHTS, 0, 0, &world->direct_lights.count, get_uniform_type_size_gpu_aligned(UNIFORM_U32));
    set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 0, 0, &world->point_lights.count, get_uniform_type_size_gpu_aligned(UNIFORM_U32));

    For (world->direct_lights) {
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const vec3 light_direction = get_forward_vector(it.rotation);

        // @Speed: its a bit painful to see several set calls instead of just one.
        // @Cleanup: figure out to make it cleaner, maybe get rid of field index parameters;
        // 0 field index is light count, so skipped.
        set_uniform_block_value(UNIFORM_BLOCK_DIRECT_LIGHTS, 1, it.u_light_index, &light_direction, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        set_uniform_block_value(UNIFORM_BLOCK_DIRECT_LIGHTS, 2, it.u_light_index, &it.ambient, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        set_uniform_block_value(UNIFORM_BLOCK_DIRECT_LIGHTS, 3, it.u_light_index, &it.diffuse, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        set_uniform_block_value(UNIFORM_BLOCK_DIRECT_LIGHTS, 4, it.u_light_index, &it.specular, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
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
        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 1, it.u_light_index, &it.location, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));

        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 2, it.u_light_index, &it.ambient, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 3, it.u_light_index, &it.diffuse, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));
        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 4, it.u_light_index, &it.specular, get_uniform_type_size_gpu_aligned(UNIFORM_F32_3));

        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 5, it.u_light_index, &it.attenuation.constant, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 6, it.u_light_index, &it.attenuation.linear, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
        set_uniform_block_value(UNIFORM_BLOCK_POINT_LIGHTS, 7, it.u_light_index, &it.attenuation.quadratic, get_uniform_type_size_gpu_aligned(UNIFORM_F32));
    }
    
	For (world->static_meshes) {
        const s32 mti = it.draw_data.material_index;
        const mat4 model = mat4_transform(it.location, it.rotation, it.scale);
		set_material_uniform_value(mti, "u_model", &model);

        const auto *material = render_registry.materials.find(mti);
        if (material) {
            set_material_uniform_value(mti, "u_material.ambient",   &material->ambient);
            set_material_uniform_value(mti, "u_material.diffuse",   &material->diffuse);
            set_material_uniform_value(mti, "u_material.specular",  &material->specular);
            set_material_uniform_value(mti, "u_material.shininess", &material->shininess);
        }
        
        auto &aabb = world->aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;

        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        // @Todo: take into account rotation and scale.
	}

    auto &player = world->player;
    
    {   // Tick player.
        const auto last_move_direction = player.move_direction;
    
        if (game_state.mode == MODE_GAME) {
            // @Cleanup: create sort of input stack to determine where to pass events.
            if (debug_console.is_open) {
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
        }

        player.collide_aabb_index = INVALID_INDEX;
        auto &player_aabb = world->aabbs[player.aabb_index];
    
        for (s32 i = 0; i < world->aabbs.count; ++i) {
            if (i == player.aabb_index) continue;
        
            const auto &aabb = world->aabbs[i];
            const vec3 resolved_velocity = resolve_moving_static(player_aabb, aabb, player.velocity);
            if (resolved_velocity != player.velocity) {
                player.velocity = resolved_velocity;
                player.collide_aabb_index = i;
                break;
            }
        }

        if (player.velocity == vec3_zero) {
            const auto &asset = asset_table[texture_sids.player_idle[player.move_direction]];
            render_registry.materials[player.draw_data.material_index].texture_index = asset.registry_index;
        } else {
            player.flip_book = &flip_books.player_move[player.move_direction];
            tick(player.flip_book, dt);

            const auto &asset = asset_table[get_current_frame(player.flip_book)];
            render_registry.materials[player.draw_data.material_index].texture_index = asset.registry_index;
        }
            
        player.location += player.velocity;
    
        const auto aabb_offset = vec3(player.scale.x * 0.5f, 0.0f, player.scale.x * 0.3f);
        player_aabb.min = player.location - aabb_offset;
        player_aabb.max = player.location + aabb_offset + vec3(0.0f, player.scale.y, 0.0f);

        if (player.velocity == vec3_zero) {
            stop_sound(player.steps_sid);
        } else {
            play_sound_or_continue(player.steps_sid);
        }

        const s32 mti = player.draw_data.material_index;
        const mat4 model = mat4_transform(player.location, player.rotation, player.scale);
        set_material_uniform_value(mti, "u_model", &model);

        const auto *material = render_registry.materials.find(mti);
        if (material) {
            set_material_uniform_value(mti, "u_material.ambient",   &material->ambient);
            set_material_uniform_value(mti, "u_material.diffuse",   &material->diffuse);
            set_material_uniform_value(mti, "u_material.specular",  &material->specular);
            set_material_uniform_value(mti, "u_material.shininess", &material->shininess);
        }
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
        } else if (game_state.mode == MODE_EDITOR) {
            if (!debug_console.is_open) {
                const f32 mouse_sensitivity = player.mouse_sensitivity;
                auto &camera = world->ed_camera;

                if (window->cursor_locked) {   
                    camera.yaw += input_table.mouse_offset_x * mouse_sensitivity * dt;
                    camera.pitch += input_table.mouse_offset_y * mouse_sensitivity * dt;
                    camera.pitch = Clamp(camera.pitch, -89.0f, 89.0f);
                }
                    
                const f32 speed = player.ed_camera_speed * dt;
                const vec3 camera_forward = forward(camera.yaw, camera.pitch);
                const vec3 camera_right = camera.up.cross(camera_forward).normalize();

                vec3 velocity;

                if (input_table.key_states[KEY_D])
                    velocity += speed * camera_right;

                if (input_table.key_states[KEY_A])
                    velocity -= speed * camera_right;

                if (input_table.key_states[KEY_W])
                    velocity += speed * camera_forward;

                if (input_table.key_states[KEY_S])
                    velocity -= speed * camera_forward;

                if (input_table.key_states[KEY_E])
                    velocity += speed * camera.up;

                if (input_table.key_states[KEY_Q])
                    velocity -= speed * camera.up;

                camera.eye += velocity.truncate(speed);
                camera.at = camera.eye + camera_forward;
            }
        }
    }
    
	if (game_state.mode == MODE_GAME) {
		world->ed_camera = world->camera;
	} else 	if (game_state.mode == MODE_EDITOR) {
        const bool ctrl_down  = input_table.key_states[KEY_CTRL];
        const bool shift_down = input_table.key_states[KEY_SHIFT];
        
        if (world->mouse_picked_entity) {
            auto *e = world->mouse_picked_entity;

            if (game_state.selected_entity_property_to_change == PROPERTY_ROTATION) {
                const f32 rotate_speed = shift_down ? 0.04f : 0.01f;

                if (input_table.key_states[KEY_LEFT]) {
                    e->rotation *= quat_from_axis_angle(vec3_left, rotate_speed);
                }

                if (input_table.key_states[KEY_RIGHT]) {
                    e->rotation *= quat_from_axis_angle(vec3_right, rotate_speed);
                }

                if (input_table.key_states[KEY_UP]) {
                    const vec3 direction = ctrl_down ? vec3_up : vec3_forward;
                    e->rotation *= quat_from_axis_angle(direction, rotate_speed);
                }

                if (input_table.key_states[KEY_DOWN]) {
                    const vec3 direction = ctrl_down ? vec3_down : vec3_back;
                    e->rotation *= quat_from_axis_angle(direction, rotate_speed);
                }
            } else if (game_state.selected_entity_property_to_change == PROPERTY_SCALE) {
                const f32 scale_speed = shift_down ? 4.0f : 1.0f;
                
                if (input_table.key_states[KEY_LEFT]) {
                    e->scale += scale_speed * dt * vec3_left;
                }

                if (input_table.key_states[KEY_RIGHT]) {
                    e->scale += scale_speed * dt * vec3_right;
                }

                if (input_table.key_states[KEY_UP]) {
                    const vec3 direction = ctrl_down ? vec3_up : vec3_forward;
                    e->scale += scale_speed * dt * direction;
                }

                if (input_table.key_states[KEY_DOWN]) {
                    const vec3 direction = ctrl_down ? vec3_down : vec3_back;
                    e->scale += scale_speed * dt * direction;
                }
            } else if (game_state.selected_entity_property_to_change == PROPERTY_LOCATION) {
                const f32 move_speed = shift_down ? 4.0f : 1.0f;

                if (input_table.key_states[KEY_LEFT]) {
                    e->location += move_speed * dt * vec3_left;
                }

                if (input_table.key_states[KEY_RIGHT]) {
                    e->location += move_speed * dt * vec3_right;
                }

                if (input_table.key_states[KEY_UP]) {
                    const vec3 direction = ctrl_down ? vec3_up : vec3_forward;
                    e->location += move_speed * dt * direction;
                }

                if (input_table.key_states[KEY_DOWN]) {
                    const vec3 direction = ctrl_down ? vec3_down : vec3_back;
                    e->location += move_speed * dt * direction;
                }
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

s32 create_static_mesh(World *world) {
    const s32 index = world->static_meshes.add_default();
    world->static_meshes[index].aabb_index = world->aabbs.add_default();
    return index;
}

Entity *find_entity_by_id(World* world, s32 id) {
    struct Find_Entity_By_Id_Data {
        Entity *e = null;
        s32 id = INVALID_INDEX;
    };

    static const auto cb_find_entity_by_id = [] (Entity *e, void *user_data) -> For_Each_Result {
        auto *data = (Find_Entity_By_Id_Data *)user_data;
    
        if (e->id == data->id) {
            data->e = e;
            return RESULT_BREAK;
        }

        return RESULT_CONTINUE;
    };

    Find_Entity_By_Id_Data find_data;
    find_data.e  = null;
    find_data.id = id;
    
    for_each_entity(world, cb_find_entity_by_id, &find_data);

    if (!find_data.e) {
        warn("Haven't found entity in world by id %d", id);
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
}
