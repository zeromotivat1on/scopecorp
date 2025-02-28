#include "pch.h"
#include "log.h"
#include "font.h"
#include "flip_book.h"
#include "memory_storage.h"

#include "os/file.h"
#include "os/thread.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"
#include "os/wgl.h"

#include "render/gl.h"
#include "render/draw.h"
#include "render/text.h"
#include "render/render_registry.h"
#include "render/viewport.h"

#include "audio/al.h"
#include "audio/alc.h"
#include "audio/sound.h"

#include "game/world.h"
#include "game/game.h"
#include "editor/hot_reload.h"

#include <stdio.h>
#include <string.h>

static void on_window_event(Window* window, Window_Event* event) {    
    handle_event(window, event);
}

int main() {
    prealloc_root();

    constexpr u64 persistent_memory_size = MB(64);
    constexpr u64 frame_memory_size = MB(16);
    constexpr u64 temp_memory_size = MB(64);
    prealloc_persistent(persistent_memory_size);
    prealloc_frame(frame_memory_size);
    prealloc_temp(temp_memory_size);

    log("Preallocated memory storages: Persistent %.fmb | Frame %.fmb | Temp %.fmb",
        (f32)persistent_memory_size / 1024 / 1024, (f32)frame_memory_size / 1024 / 1024, (f32)temp_memory_size / 1024 / 1024);
    
    init_input_table();
    
    window = create_window(1280, 720, "Scopecorp", 0, 0);
    if (!window) {
        error("Failed to create window");
        return 1;
    }

    register_event_callback(window, on_window_event);

    // @Cleanup: not sure if its a good idea to pass gl major/minor version here.
    wgl_init(window, 4, 6);
    wgl_vsync(false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);  
    glFrontFace(GL_CCW);

    glEnable(GL_SCISSOR_TEST);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    ALCdevice* audio_device = alcOpenDevice(null);
    if (!audio_device) {
        error("Failed to open default audio device");
        return 1;
    }

    ALCcontext* audio_context = alcCreateContext(audio_device, null);
    if (!audio_context) {
        error("Failed to create alc context");
        return 1;
    }

    if (!alcMakeContextCurrent(audio_context)) {
        error("Failed to make alc context current");
        return 1;
    }

    init_render_registry(&render_registry);    
    load_game_textures(&texture_index_list);
    compile_game_shaders(&shader_index_list);
    create_game_materials(&material_index_list);

    create_game_flip_books(&flip_books);
    load_game_sounds(&sounds);

    //alSourcePlay(sounds.world.source);

    init_shader_hot_reload(&shader_hot_reload_queue);
    
    Hot_Reload_List hot_reload_list = {0};
    register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);
    start_hot_reload_thread(&hot_reload_list);

    Font* font = create_font(DIR_FONTS "consola.ttf");
    Font_Atlas* atlas = bake_font_atlas(font, 32, 128, 16);
    text_draw_cmd = create_default_text_draw_command(atlas);
    
    init_draw_queue(&draw_queue);
    
    world = create_world();

    const auto& player_idle_texture = render_registry.textures[texture_index_list.player_idle[BACK]];
    const f32 player_scale_aspect = (f32)player_idle_texture.width / player_idle_texture.height;
    const f32 player_y_scale = 1.0f * player_scale_aspect;
    const f32 player_x_scale = player_y_scale * player_scale_aspect;

    Player& player = world->player;
    player.scale = vec3(player_x_scale, player_y_scale, 1.0f);
    
    Camera& camera = world->camera;
    camera.mode = MODE_PERSPECTIVE;
    camera.yaw = 90.0f;
	camera.pitch = 0.0f;
	camera.eye = player.location + player.camera_offset;
	camera.at = camera.eye + forward(camera.yaw, camera.pitch);
	camera.up = vec3(0.0f, 1.0f, 0.0f);
	camera.fov = 60.0f;
	//camera.aspect = window->width / window->height;
	//camera.aspect = 4.0f / 3.0f;
	camera.near = 0.1f;
	camera.far = 1000.0f;
	camera.left = 0.0f;
	camera.right = (f32)window->width;
	camera.bottom = 0.0f;
	camera.top = (f32)window->height;

    world->ed_camera = camera;

    Draw_Command ground_draw_cmd;
    {   // Create ground.
        ground_draw_cmd.material_idx = material_index_list.ground;

        Vertex_PU vertices[] = {
            { vec3( 0.5f,  0.5f, 0.0f), vec2(1.0f, 1.0f) },
            { vec3( 0.5f, -0.5f, 0.0f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 0.0f) },
            { vec3(-0.5f,  0.5f, 0.0f), vec2(0.0f, 1.0f) },
        };
        Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
        ground_draw_cmd.vertex_buffer_idx = create_vertex_buffer(attribs, c_array_count(attribs), (f32*)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);
        
        u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
        ground_draw_cmd.index_buffer_idx = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
    }
    
    Draw_Command player_draw_cmd;
    {   // Create player.
        player_draw_cmd.material_idx = material_index_list.player;

        Vertex_PU vertices[4] = { // center in bottom mid point of quad
            { vec3( 0.5f,  1.0f, 0.0f), vec2(1.0f, 1.0f) },
            { vec3( 0.5f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
            { vec3(-0.5f,  1.0f, 0.0f), vec2(0.0f, 1.0f) },
        };
        Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
        player_draw_cmd.vertex_buffer_idx = create_vertex_buffer(attribs, c_array_count(attribs), (f32*)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

        u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
        player_draw_cmd.index_buffer_idx = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
    }

    Draw_Command cube_draw_cmd;
    {   // Create cube.
        cube_draw_cmd.material_idx = material_index_list.cube;

        Vertex_PU vertices[] = {
            // Front face
            { vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 0.0f) },
            { vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f) },
            { vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

            // Back face
            { vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
            { vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
            { vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 1.0f) },

            // Left face
            { vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
            { vec3(-0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
            { vec3(-0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

            // Right face
            { vec3( 0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
            { vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 0.0f) },
            { vec3( 0.5f, -0.5f, -0.5f), vec2(0.0f, 1.0f) },
            { vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },

            // Top face
            { vec3(-0.5f,  0.5f, -0.5f), vec2(0.0f, 0.0f) },
            { vec3( 0.5f,  0.5f, -0.5f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f,  0.5f,  0.5f), vec2(0.0f, 1.0f) },
            { vec3( 0.5f,  0.5f,  0.5f), vec2(1.0f, 1.0f) },

            // Bottom face
            { vec3(-0.5f, -0.5f, -0.5f), vec2(0.0f, 0.0f) },
            { vec3( 0.5f, -0.5f, -0.5f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f, -0.5f,  0.5f), vec2(0.0f, 1.0f) },
            { vec3( 0.5f, -0.5f,  0.5f), vec2(1.0f, 1.0f) },
        };
        Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
        cube_draw_cmd.vertex_buffer_idx = create_vertex_buffer(attribs, c_array_count(attribs), (f32*)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);

        u32 indices[] = {
            // Front face
             0,  1,  2,  1,  3,  2,
            // Back face
             4,  6,  5,  5,  6,  7,
            // Left face
             8,  9, 10,  9, 11, 10,
            // Right face
            12, 14, 13, 13, 14, 15,
            // Bottom face
            16, 17, 18, 17, 19, 18,
            // Top face
            20, 22, 21, 21, 22, 23
        };
        cube_draw_cmd.index_buffer_idx = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
    }

    Draw_Command skybox_draw_cmd;
    {   // Create skybox.
        skybox_draw_cmd.flags |= DRAW_FLAG_IGNORE_DEPTH;
        skybox_draw_cmd.material_idx = material_index_list.skybox;

        Vertex_PU vertices[] = {
            { vec3( 1.0f,  1.0f, 0.0f), vec2(1.0f, 1.0f) },
            { vec3( 1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f) },
            { vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f) },
            { vec3(-1.0f,  1.0f, 0.0f), vec2(0.0f, 1.0f) },
        };
        Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V3, VERTEX_ATTRIB_F32_V2 };
        skybox_draw_cmd.vertex_buffer_idx = create_vertex_buffer(attribs, c_array_count(attribs), (f32*)vertices, 5 * c_array_count(vertices), BUFFER_USAGE_STATIC);
        
        u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
        skybox_draw_cmd.index_buffer_idx = create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
    }
    
    f32 dt = 0.0f;
    s64 begin_counter = performance_counter();
    const f32 frequency = (f32)performance_frequency();

    while (alive(window)) {
        poll_events(window);
        tick(world, dt);

        // @Cleanup: temp listener position update here.
        alListener3f(AL_POSITION, player.location.x, player.location.y, player.location.z);

        check_shader_hot_reload_queue(&shader_hot_reload_queue);
        
        glClearColor(0.9f, 0.4f, 0.5f, 1.0f); // ugly bright pink
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        const Camera* current_camera = desired_camera(world);
        
        // Draw skybox.
        const vec2 scale = vec2(8.0f, 4.0f);
        set_material_uniform_value(skybox_draw_cmd.material_idx, "u_scale", &scale);
        set_material_uniform_value(skybox_draw_cmd.material_idx, "u_offset", &camera.eye);
        enqueue_draw_command(&draw_queue, &skybox_draw_cmd);

        // Draw ground.
        const vec3 ground_scale = vec3(16);
        const mat4 ground_m = mat4_transform(vec3(0.0f), quat_from_axis_angle(vec3_right, 90), ground_scale);
        const mat4 ground_v = camera_view(current_camera);
        const mat4 ground_p = camera_projection(current_camera);
        const mat4 ground_mvp = ground_m * ground_v * ground_p;
        set_material_uniform_value(ground_draw_cmd.material_idx, "u_mvp", ground_mvp.ptr());
        set_material_uniform_value(ground_draw_cmd.material_idx, "u_scale", &ground_scale);
        enqueue_draw_command(&draw_queue, &ground_draw_cmd);

        // Draw cube.
        const mat4 cube_m = mat4_transform(vec3(3.0f, 0.5f, 4.0f), quat(), vec3(1.0f));
        const mat4 cube_v = camera_view(current_camera);
        const mat4 cube_p = camera_projection(current_camera);
        const mat4 cube_mvp = cube_m * cube_v * cube_p;
        set_material_uniform_value(cube_draw_cmd.material_idx, "u_mvp", cube_mvp.ptr());
        enqueue_draw_command(&draw_queue, &cube_draw_cmd);
        
        // Draw player.        
        const mat4 player_m = mat4_transform(player.location, player.rotation, player.scale);
        const mat4 player_v = camera_view(current_camera);
        const mat4 player_p = camera_projection(current_camera);
        const mat4 player_mvp = player_m * player_v * player_p;
        set_material_uniform_value(player_draw_cmd.material_idx, "u_mvp", player_mvp.ptr());
        // @Cleanup: create function for this.
        render_registry.materials[player_draw_cmd.material_idx].texture_idx = player.texture_idx;
        enqueue_draw_command(&draw_queue, &player_draw_cmd);
        
        // @Cleanup: flush before text draw as its overwritten by skybox, fix.
        flush_draw_commands(&draw_queue);

        static char text[256];
        const vec3 text_color = vec3(1.0f);
        const f32 padding = atlas->font_size * 0.5f;
        const vec2 shadow_offset = vec2(atlas->font_size * 0.1f, -atlas->font_size * 0.1f);
        s32 text_size = 0;
        vec2 pos;

        {   // Entity data.
            text_size = (s32)sprintf_s(text, sizeof(text), "player location %s velocity %s", to_string(player.location), to_string(player.velocity));
            pos.x = padding;
            pos.y = (f32)viewport.height - atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
            
            text_size = (s32)sprintf_s(text, sizeof(text), "camera eye %s at %s", to_string(current_camera->eye), to_string(current_camera->at));
            pos.x = padding;
            pos.y -= atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
        }
        
        {   // Runtime stats.
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d %s", dt * 1000.0f, 1 / dt, window->width, window->height, build_type_name);
            pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            pos.y = viewport.height - (f32)atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
        }

        {   // Controls.
            text_size = (s32)sprintf_s(text, sizeof(text), "F1 %s F2 %s F3 %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
            pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            pos.y = padding;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);

            text_size = (s32)sprintf_s(text, sizeof(text), "Shift/Control + Arrows - force move/rotate game camera");
            pos.x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            pos.y += atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
        }

        {   // Memory stats.
            u64 persistent_size;
            u64 persistent_used;
            usage_persistent(&persistent_size, &persistent_used);
            f32 persistent_part = (f32)persistent_used / persistent_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)persistent_used / 1024 / 1024, (f32)persistent_size / 1024 / 1024, persistent_part);
            pos.x = padding;
            pos.y = padding;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
            
            u64 frame_size;
            u64 frame_used;
            usage_frame(&frame_size, &frame_used);
            f32 frame_part = (f32)frame_used / frame_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame_used / 1024 / 1024, (f32)frame_size / 1024 / 1024, frame_part);
            pos.x = padding;
            pos.y += atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
            
            u64 temp_size;
            u64 temp_used;
            usage_temp(&temp_size, &temp_used);
            f32 temp_part = (f32)temp_used / temp_size * 100.0f;
            
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp_used / 1024 / 1024, (f32)temp_size / 1024 / 1024, temp_part);
            pos.x = padding;
            pos.y += atlas->font_size;
            draw_text_immediate_with_shadow(text_draw_cmd, text, text_size, pos, text_color, shadow_offset, vec3_zero);
        }

        wgl_swap_buffers(window);
        free_all_frame();
        
        const s64 end_counter = performance_counter();
        dt = (end_counter - begin_counter) / frequency;
		begin_counter = end_counter;

#if DEBUG
        // If dt is too large, we could have resumed from a breakpoint.
        if (dt > 1.0f) dt = 0.16f;
#endif
    }

    stop_hot_reload_thread(&hot_reload_list);
    destroy(window);
    free_root();
    
    return 0;
}
