#include "pch.h"
#include "game/game.h"
#include "game/world.h"

#include "str.h"
#include "log.h"
#include "profile.h"
#include "input_stack.h"
#include "asset.h"

#include "math/math_basic.h"

#include "os/file.h"
#include "os/input.h"
#include "os/window.h"

#include "editor/editor.h"
#include "editor/debug_console.h"
#include "editor/telemetry.h"

#include "render/render.h"
#include "render/ui.h"
#include "render/r_table.h"
#include "render/r_target.h"
#include "render/r_pass.h"
#include "render/r_viewport.h"
#include "render/r_uniform.h"
#include "render/r_material.h"
#include "render/r_texture.h"
#include "render/r_storage.h"
#include "render/r_flip_book.h"

#include "audio/audio.h"
#include "audio/au_sound.h"

void on_window_resize(u16 width, u16 height) {
    r_resize_viewport(R_viewport, width, height);
    R_viewport.orthographic_projection = mat4_orthographic(0, R_viewport.width, 0, R_viewport.height, -1, 1);

    const vec2 viewport_resolution = vec2(R_viewport.width, R_viewport.height);
    r_set_uniform_block_value(&uniform_block_viewport, 0, 0, &viewport_resolution, r_uniform_type_size_gpu_aligned(R_F32_2));        
    r_set_uniform_block_value(&uniform_block_viewport, 1, 0, &R_viewport.orthographic_projection, r_uniform_type_size_gpu_aligned(R_F32_4X4));        
        
    on_viewport_resize(World.camera, R_viewport);
    World.ed_camera.aspect = World.camera.aspect;
    World.ed_camera.left   = World.camera.left;
    World.ed_camera.right  = World.camera.right;
    World.ed_camera.bottom = World.camera.bottom;
    World.ed_camera.top    = World.camera.top;

    on_viewport_resize_debug_console(R_viewport.width, R_viewport.height);

    const auto &rt = R_table.targets[R_viewport.render_target];
    const auto &mta = *find_asset(SID_MATERIAL_FRAME_BUFFER);
    const vec2 resolution = vec2(rt.width, rt.height);
    r_set_material_uniform(mta.index, SID("u_resolution"),
                           0, sizeof(resolution), &resolution);
}

void on_input_game(const Window_Event &event) {
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
            tm_open();
        } else if (press && key == KEY_SWITCH_MEMORY_PROFILER) {
            open_memory_profiler();
        } else if (press && key == KEY_SWITCH_EDITOR_MODE) {
            game_state.mode = MODE_EDITOR;
            push_input_layer(Input_layer_editor);
            editor_report("Editor");
        }  else if (press && key == KEY_SWITCH_POLYGON_MODE) {
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
        } else if (press && key == KEY_1) {
            game_state.camera_behavior = (Camera_Behavior)(((s32)game_state.camera_behavior + 1) % 3);
        } else if (press && key == KEY_2) {
            game_state.player_movement_behavior = (Player_Movement_Behavior)(((s32)game_state.player_movement_behavior + 1) % 2);
        }

        break;
	}
    }
}

void create_world(Game_World &w) {
    w = {};
    
    reserve(w.arena, MB(8));
    
	sparse_reserve(w.arena, w.static_meshes,     w.MAX_STATIC_MESHES);
    sparse_reserve(w.arena, w.point_lights,      w.MAX_POINT_LIGHTS);
    sparse_reserve(w.arena, w.direct_lights,     w.MAX_DIRECT_LIGHTS);
    sparse_reserve(w.arena, w.sound_emitters_2d, w.MAX_SOUND_EMITTERS_2D);
    sparse_reserve(w.arena, w.sound_emitters_3d, w.MAX_SOUND_EMITTERS_3D);
    sparse_reserve(w.arena, w.portals,           w.MAX_PORTALS);
    sparse_reserve(w.arena, w.aabbs,             w.MAX_AABBS);
}

template <typename T>
static void read_sparse_array(File file, Sparse<T> *array) {
    // @Cleanup: read sparse array directly with replace or add read data to existing one?

    os_read_file(file, _sizeref(array->count));

    s32 capacity = 0;
    os_read_file(file, _sizeref(capacity));
    Assert(capacity <= array->capacity);

    os_read_file(file, capacity * sizeof(T),   array->items);
    os_read_file(file, capacity * sizeof(s32), array->dense);
    os_read_file(file, capacity * sizeof(s32), array->sparse);
}

template <typename T>
static void write_sparse_array(File file, Sparse<T> *array) {
    os_write_file(file, _sizeref(array->count));
    os_write_file(file, _sizeref(array->capacity));
    os_write_file(file, array->capacity * sizeof(T),   array->items);
    os_write_file(file, array->capacity * sizeof(s32), array->dense);
    os_write_file(file, array->capacity * sizeof(s32), array->sparse);
}

void save_level(Game_World &world) {
    START_SCOPE_TIMER(save);

    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    String_Builder sb;
    str_build(scratch.arena, sb, DIR_LEVELS);
    str_build(scratch.arena, sb, world.name);

    String path = str_build_finish(scratch.arena, sb);
    
    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_WRITE_BIT);
    defer { os_close_file(file); };
    
    if (file == FILE_NONE) {
        log("Level %s does not exist, creating new one", path);
        file = os_open_file(path, FILE_OPEN_NEW, FILE_WRITE_BIT);
        if (file == FILE_NONE) {
            error("Failed to create new level %s", path);
            return;
        }
    }

    os_write_file(file, world.MAX_NAME_SIZE, &world.name);
    
    os_write_file(file, _sizeref(world.player));
    os_write_file(file, _sizeref(world.camera));
    os_write_file(file, _sizeref(world.ed_camera));
    os_write_file(file, _sizeref(world.skybox));

    write_sparse_array(file, &world.static_meshes);
    write_sparse_array(file, &world.point_lights);
    write_sparse_array(file, &world.direct_lights);
    write_sparse_array(file, &world.sound_emitters_2d);
    write_sparse_array(file, &world.sound_emitters_3d);
    write_sparse_array(file, &world.portals);
    write_sparse_array(file, &world.aabbs);
    
    log("Saved level %.*s in %.2fms", path.length, path.value, CHECK_SCOPE_TIMER_MS(save));
    editor_report("Saved level %.*s", path.length, path.value);
}

static For_Result cb_init_entity_after_level_load(Entity *e, void *user_data) {
    auto *world = (Game_World *)user_data;

    if (e->eid != EID_NONE) {        
        e->draw_data.eid_offset = head_pointer(R_eid_alloc_range);
        *(eid *)r_alloc(R_eid_alloc_range, sizeof(eid)) = e->eid;
    }

    if (e->eid > eid_global_counter) {
        eid_global_counter = e->eid + 1;
    }
    
    return CONTINUE;
}

void load_level(Game_World &world, String path) {
    START_SCOPE_TIMER(load);

    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_READ_BIT);
    defer { os_close_file(file); };
    
    if (file == FILE_NONE) {
        error("Failed to open level %s", path);
        return;
    }

    os_read_file(file, world.MAX_NAME_SIZE, &world.name);
    
    os_read_file(file, _sizeref(world.player));
    os_read_file(file, _sizeref(world.camera));
    os_read_file(file, _sizeref(world.ed_camera));
    os_read_file(file, _sizeref(world.skybox));

    read_sparse_array(file, &world.static_meshes);
    read_sparse_array(file, &world.point_lights);
    read_sparse_array(file, &world.direct_lights);
    read_sparse_array(file, &world.sound_emitters_2d);
    read_sparse_array(file, &world.sound_emitters_3d);
    read_sparse_array(file, &world.portals);
    read_sparse_array(file, &world.aabbs);

    for_each_entity(world, cb_init_entity_after_level_load, &world);
    
    log("Loaded level %s in %.2fms", path, CHECK_SCOPE_TIMER_MS(load));
    editor_report("Loaded level %s", path);
}

void tick_game(f32 dt) {
    TM_SCOPE_ZONE(__func__);

    // @Cleanup: looks more like a hack.
    const auto *input_layer = get_current_input_layer();
    if (!input_layer) {
        push_input_layer(Input_layer_game);
        input_layer = get_current_input_layer();
    }
    
    auto &camera = active_camera(World);
    auto &player = World.player;
    auto &skybox = World.skybox;
    
	World.dt = dt;

    au_set_listener_pos(player.location);

    const auto &sna = *find_asset(SID_SOUND_WIND_AMBIENCE);
    au_play_sound_or_continue(sna.index, player.location);

    update_matrices(camera);

    r_set_uniform_block_value(&uniform_block_camera, 0, 0, &camera.eye,       r_uniform_type_size_gpu_aligned(R_F32_3));
    r_set_uniform_block_value(&uniform_block_camera, 1, 0, &camera.view,      r_uniform_type_size_gpu_aligned(R_F32_4X4));
    r_set_uniform_block_value(&uniform_block_camera, 2, 0, &camera.proj,      r_uniform_type_size_gpu_aligned(R_F32_4X4));
    r_set_uniform_block_value(&uniform_block_camera, 3, 0, &camera.view_proj, r_uniform_type_size_gpu_aligned(R_F32_4X4));

    {   // Update skybox.
        auto &aabb = World.aabbs[skybox.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = skybox.location - half_extent;
        aabb.max = skybox.location + half_extent;
        
        auto *mta = find_asset(skybox.draw_data.sid_material);
        if (mta) {
            skybox.uv_offset = camera.eye;

            const auto index = mta->index;
            r_set_material_uniform(index, SID("u_uv_scale"),  0, _sizeref(skybox.uv_scale));
            r_set_material_uniform(index, SID("u_uv_offset"), 0, _sizeref(skybox.uv_offset));
        }
    }

    r_set_uniform_block_value(&uniform_block_direct_lights, 0, 0, &World.direct_lights.count, r_uniform_type_size_gpu_aligned(R_U32));
    r_set_uniform_block_value(&uniform_block_point_lights, 0, 0, &World.point_lights.count, r_uniform_type_size_gpu_aligned(R_U32));

    TM_PUSH_ZONE("direct_lights");
    For (World.direct_lights) {
        auto &aabb = World.aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const vec3 light_direction = forward(it.rotation);

        // @Speed: its a bit painful to see several set calls instead of just one.
        // @Cleanup: figure out to make it cleaner, maybe get rid of field index parameters;
        // 0 field index is light count, so skipped.
        r_set_uniform_block_value(&uniform_block_direct_lights, 1, it.u_light_index, &light_direction, r_uniform_type_size_gpu_aligned(R_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 2, it.u_light_index, &it.ambient, r_uniform_type_size_gpu_aligned(R_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 3, it.u_light_index, &it.diffuse, r_uniform_type_size_gpu_aligned(R_F32_3));
        r_set_uniform_block_value(&uniform_block_direct_lights, 4, it.u_light_index, &it.specular, r_uniform_type_size_gpu_aligned(R_F32_3));
    }
    TM_POP_ZONE();

    TM_PUSH_ZONE("point_lights");
    For (World.point_lights) {
        auto &aabb = World.aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const vec3 light_direction = vec3_zero;
        
        // @Speed: its a bit painful to see several set calls instead of just one.
        // @Cleanup: figure out to make it cleaner, maybe get rid of field index parameters;
        // 0 field index is light count, so skipped.
        r_set_uniform_block_value(&uniform_block_point_lights, 1, it.u_light_index, &it.location, r_uniform_type_size_gpu_aligned(R_F32_3));

        r_set_uniform_block_value(&uniform_block_point_lights, 2, it.u_light_index, &it.ambient, r_uniform_type_size_gpu_aligned(R_F32_3));
        r_set_uniform_block_value(&uniform_block_point_lights, 3, it.u_light_index, &it.diffuse, r_uniform_type_size_gpu_aligned(R_F32_3));
        r_set_uniform_block_value(&uniform_block_point_lights, 4, it.u_light_index, &it.specular, r_uniform_type_size_gpu_aligned(R_F32_3));

        r_set_uniform_block_value(&uniform_block_point_lights, 5, it.u_light_index, &it.attenuation.constant, r_uniform_type_size_gpu_aligned(R_F32));
        r_set_uniform_block_value(&uniform_block_point_lights, 6, it.u_light_index, &it.attenuation.linear, r_uniform_type_size_gpu_aligned(R_F32));
        r_set_uniform_block_value(&uniform_block_point_lights, 7, it.u_light_index, &it.attenuation.quadratic, r_uniform_type_size_gpu_aligned(R_F32));
    }
    TM_POP_ZONE();

    TM_PUSH_ZONE("static_meshes");
	For (World.static_meshes) {
        // @Todo: take into account rotation and scale.
        auto &aabb = World.aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        if (it.draw_data.sid_material == SID_NONE) continue;
        
        auto *mta = find_asset(it.draw_data.sid_material);
        if (!mta) continue;

        const auto &mt = R_table.materials[mta->index];
        const mat4 model = mat4_transform(it.location, it.rotation, it.scale);
        
		r_set_material_uniform(mta->index, SID("u_model"),    0, _sizeref(model));
        r_set_material_uniform(mta->index, SID("u_uv_scale"), 0, _sizeref(it.uv_scale));

        r_set_material_uniform(mta->index, SID("u_material.ambient"),
                               0, _sizeref(mt.light_params.ambient));
        r_set_material_uniform(mta->index, SID("u_material.diffuse"),
                               0, _sizeref(mt.light_params.diffuse));
        r_set_material_uniform(mta->index, SID("u_material.specular"),
                               0, _sizeref(mt.light_params.specular));
        r_set_material_uniform(mta->index, SID("u_material.shininess"),
                               0, _sizeref(mt.light_params.shininess));
	}
    TM_POP_ZONE();

    // @Todo: fine-tuned sound play.
    
    For (World.sound_emitters_2d) {
        auto &aabb = World.aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const auto &sna = *find_asset(it.sid_sound);
        au_play_sound_or_continue(sna.index, au_get_listener_pos());
    }

    For (World.sound_emitters_3d) {
        auto &aabb = World.aabbs[it.aabb_index];
        const vec3 half_extent = (aabb.max - aabb.min) * 0.5f;
        aabb.min = it.location - half_extent;
        aabb.max = it.location + half_extent;

        const auto &sna = *find_asset(it.sid_sound);
        au_play_sound_or_continue(sna.index, it.location);
    }
    
    {   // Tick player.
        if (input_layer->type != INPUT_LAYER_GAME) {
            player.velocity = vec3_zero;
        } else {
            const f32 speed = player.move_speed * dt;
            vec3 velocity;

            // @Todo: use input action instead of direct key state.
            if (game_state.player_movement_behavior == MOVE_INDEPENDENT) {
                if (down(KEY_D)) {
                    velocity.x = speed;
                    player.move_direction = EAST;
                }

                if (down(KEY_A)) {
                    velocity.x = -speed;
                    player.move_direction = WEST;
                }

                if (down(KEY_W)) {
                    velocity.z = speed;
                    player.move_direction = NORTH;
                }

                if (down(KEY_S)) {
                    velocity.z = -speed;
                    player.move_direction = SOUTH;
                }
            } else if (game_state.player_movement_behavior == MOVE_RELATIVE_TO_CAMERA) {
                auto &camera = World.camera;
                const vec3 camera_forward = forward(camera.yaw, camera.pitch);
                const vec3 camera_right = normalize(cross(camera.up, camera_forward));

                if (down(KEY_D)) {
                    velocity += speed * camera_right;
                    player.move_direction = EAST;
                }

                if (down(KEY_A)) {
                    velocity -= speed * camera_right;
                    player.move_direction = WEST;
                }

                if (down(KEY_W)) {
                    velocity += speed * camera_forward;
                    player.move_direction = NORTH;
                }

                if (down(KEY_S)) {
                    velocity -= speed * camera_forward;
                    player.move_direction = SOUTH;
                }
            }

            player.velocity = truncate(velocity, speed);
        }

        player.collide_aabb_index = INDEX_NONE;
        auto &player_aabb = World.aabbs[player.aabb_index];
        
        for (s32 i = 0; i < World.aabbs.count; ++i) {
            if (i == player.aabb_index) continue;

            const auto &aabb = World.aabbs[i];
            const vec3 resolved_velocity = resolve_moving_static(player_aabb, aabb, player.velocity);
            if (resolved_velocity != player.velocity) {
                player.velocity = resolved_velocity;
                player.collide_aabb_index = i;
                break;
            }
        }
        
        auto *mta = find_asset(player.draw_data.sid_material);
        auto *mt  = sparse_find(R_table.materials, mta->index);
        if (player.velocity == vec3_zero) {
            if (mt) {
                const auto &txa = *find_asset(sid_texture_player_idle[player.move_direction]);
                mt->texture = txa.index;
            }
        } else {
            switch (player.move_direction) {
            case WEST: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_LEFT;
                break;
            }
            case EAST: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_RIGHT;
                break;
            }
            case SOUTH: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_BACK;
                break;
            }
            case NORTH: {
                player.sid_flip_book_move = SID_FLIP_BOOK_PLAYER_MOVE_FORWARD;
                break;
            }
            }

            const auto &fpa = *find_asset(player.sid_flip_book_move);
            r_tick_flip_book(fpa.index, dt);

            if (mt) {
                const auto &frame = r_get_current_flip_book_frame(fpa.index);
                mt->texture = frame.texture;
            }
        }
            
        player.location += player.velocity;
    
        const auto aabb_offset = vec3(player.scale.x * 0.5f, 0.0f, player.scale.x * 0.3f);
        player_aabb.min = player.location - aabb_offset;
        player_aabb.max = player.location + aabb_offset + vec3(0.0f, player.scale.y, 0.0f);

        const auto &sna = *find_asset(player.sid_sound_steps);
        if (player.velocity == vec3_zero) {
            au_stop_sound(sna.index);
        } else {
            au_play_sound_or_continue(sna.index);
        }

        if (mt) {
            const mat4 model = mat4_transform(player.location, player.rotation, player.scale);

            r_set_material_uniform(mta->index, SID("u_model"), 0, _sizeref(model));
            r_set_material_uniform(mta->index, SID("u_uv_scale"),
                                   0, _sizeref(player.uv_scale));

            r_set_material_uniform(mta->index, SID("u_material.ambient"),
                                   0, _sizeref(mt->light_params.ambient));
            r_set_material_uniform(mta->index, SID("u_material.diffuse"),
                                   0, _sizeref(mt->light_params.diffuse));
            r_set_material_uniform(mta->index, SID("u_material.specular"),
                                   0, _sizeref(mt->light_params.specular));
            r_set_material_uniform(mta->index, SID("u_material.shininess"),
                                   0, _sizeref(mt->light_params.shininess));
        }
    }

    {   // Tick camera.
        if (game_state.mode == MODE_GAME) {
            if (game_state.camera_behavior == STICK_TO_PLAYER) {
                auto &camera = World.camera;
                camera.eye = player.location + player.camera_offset;
                camera.at = camera.eye + forward(camera.yaw, camera.pitch);
            } else if (game_state.camera_behavior == FOLLOW_PLAYER) {
                auto &camera = World.camera;
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

                camera.eye = Lerp(camera.eye, target_eye, player.camera_follow_speed * dt);
                camera.at = camera.eye + forward(camera.yaw, camera.pitch);
            }
        }
    }
}

Camera &active_camera(Game_World &world) {
	if (game_state.mode == MODE_GAME)   return world.camera;
	if (game_state.mode == MODE_EDITOR) return world.ed_camera;

	error("Unknown game mode %d, returning game camera", game_state.mode);
	return world.camera;
}

Entity *create_entity(Game_World &w, Entity_Type e_type) {
    Entity *e = null;
    switch (e_type) {
    case E_PLAYER: {
        e = &w.player;
        break;
    }
    case E_SKYBOX: {
        e = &w.skybox;
        break;
    }
    case E_STATIC_MESH: {
        e = sparse_find(w.static_meshes, sparse_push(w.static_meshes));
        break;
    }
    case E_DIRECT_LIGHT: {
        e = sparse_find(w.direct_lights, sparse_push(w.direct_lights));
        break;
    }
    case E_POINT_LIGHT: {
        e = sparse_find(w.point_lights, sparse_push(w.point_lights));
        break;
    }
    case E_SOUND_EMITTER_2D: {
        e = sparse_find(w.sound_emitters_2d, sparse_push(w.sound_emitters_2d));
        break;
    }
    case E_SOUND_EMITTER_3D: {
        e = sparse_find(w.sound_emitters_3d, sparse_push(w.sound_emitters_3d));
        break;
    }
    case E_PORTAL: {
        e = sparse_find(w.portals, sparse_push(w.portals));
        break;
    }
    }

    if (e) {
        e->eid = eid_global_counter;
        e->aabb_index = sparse_push(w.aabbs);

        auto &aabb = w.aabbs[e->aabb_index];
        const vec3 half_extent = e->scale * 0.5f;
        aabb.min = e->location - half_extent;
        aabb.max = e->location + half_extent;

        e->draw_data.eid_offset = head_pointer(R_eid_alloc_range);
        *(eid *)r_alloc(R_eid_alloc_range, sizeof(eid)) = e->eid;
         
        eid_global_counter += 1;
    }
    
    return e;
}

struct Find_Entity_By_Id_Data {
    Entity *e = null;
    eid eid = EID_NONE;
};

static For_Result cb_find_entity_by_eid(Entity *e, void *user_data) {
    auto *data = (Find_Entity_By_Id_Data *)user_data;
    
    if (e->eid == data->eid) {
        data->e = e;
        return BREAK;
    }

    return CONTINUE;
 };

Entity *find_entity_by_eid(Game_World &world, eid eid) {
    Find_Entity_By_Id_Data find_data;
    find_data.e  = null;
    find_data.eid = eid;
    
    for_each_entity(world, cb_find_entity_by_eid, &find_data);

    if (!find_data.e) {
        warn("Haven't found entity in World by eid %u", eid);
    }
    
    return find_data.e;
}

void for_each_entity(Game_World &world, For_Each_Entity_Callback callback, void *user_data) {
    if (callback(&world.player, user_data) == BREAK) return;
    if (callback(&world.skybox, user_data) == BREAK) return;

    For (world.static_meshes) {
        if (callback(&it, user_data) == BREAK) return;
    }

    For (world.direct_lights) {
        if (callback(&it, user_data) == BREAK) return;
    }
    
    For (world.point_lights) {
        if (callback(&it, user_data) == BREAK) return;
    }

    For (world.sound_emitters_2d) {
        if (callback(&it, user_data) == BREAK) return;
    }

    For (world.sound_emitters_3d) {
        if (callback(&it, user_data) == BREAK) return;
    }

    For (world.portals) {
        if (callback(&it, user_data) == BREAK) return;
    }
}
