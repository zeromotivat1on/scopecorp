#include "pch.h"
#include "log.h"
#include "os/input.h"
#include "os/window.h"
#include "os/time.h"
#include "render/gl.h"
#include "render/shader.h"
#include "render/vertex.h"
#include "render/texture.h"
#include "flip_book.h"
#include "audio/al.h"
#include "audio/alc.h"
#include "audio/sound.h"
#include "font.h"
#include "os/file.h"
#include "os/thread.h"
#include "viewport.h"
#include "game/world.h"
#include "game/game.h"
#include "editor/hot_reload.h"
#include <stdio.h>
#include <string.h>

static void on_window_event(Window* window, Window_Event* event)
{
    if (event->type == EVENT_RESIZE)
    {
        const s16 window_w = window->width;
        const s16 window_h = window->height;

        viewport_4x3(&viewport, window_w, window_h);
        glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
        glScissor(viewport.x, viewport.y, viewport.width, viewport.height);

        world->camera.aspect = (f32)viewport.width / viewport.height;
        world->camera.left = viewport.x;
        world->camera.right = (f32)viewport.x + viewport.width;
        world->camera.bottom = viewport.y;
        world->camera.top = (f32)viewport.y + viewport.height;

        world->ed_camera = world->camera;

        on_framebuffer_resize(font_render_ctx, viewport.width, viewport.height);
    }
    
    handle_event(window, event);
}

int main()
{
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
    
    window = create_window(1280, 720, "Scopecorp", 32, 32);
    if (!window)
    {
        error("Failed to create window");
        return 1;
    }

    register_event_callback(window, on_window_event);

    // @Cleanup: not sure if its a good idea to pass gl major/minor version here.
    gl_init(window, 4, 6);
    gl_vsync(false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);  
    glFrontFace(GL_CCW);

    glEnable(GL_SCISSOR_TEST);

    glEnable(GL_DEPTH_TEST);
    //glDepthMask(GL_FALSE);
    glDepthFunc(GL_LESS);
    
    ALCdevice* audio_device = alcOpenDevice(null);
    if (!audio_device)
    {
        error("Failed to open default audio device");
        return 1;
    }

    ALCcontext* audio_context = alcCreateContext(audio_device, null);
    if (!audio_context)
    {
        error("Failed to create alc context");
        return 1;
    }

    if (!alcMakeContextCurrent(audio_context))
    {
        error("Failed to make alc context current");
        return 1;
    }

    load_game_sounds(&sounds);
    load_game_textures(&textures);
    compile_game_shaders(&shaders);
    create_game_flip_books(&flip_books);

    //alSourcePlay(sounds.world.source);

    init_shader_hot_reload(&shader_hot_reload_queue);
    
    Hot_Reload_List hot_reload_list = {0};
    register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);
    start_hot_reload_thread(&hot_reload_list);
    
    font_render_ctx = create_font_render_context(window->width, window->height);
    Font* font = create_font("C:/Windows/Fonts/Consola.ttf");
    Font_Atlas* atlas = bake_font_atlas(font, 32, 128, 16);

    world = create_world();
    
    const f32 player_scale_aspect = (f32)textures.player_idle[BACK].width / textures.player_idle[BACK].height;
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
    
    {   // Create player.
        glGenVertexArrays(1, &player.vao);
        glGenBuffers(1, &player.vbo);
        glGenBuffers(1, &player.ibo);
    
        glBindVertexArray(player.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, player.vbo);
        Vertex_Pos_Tex vertices[4] = { // center in bottom mid point of quad
            { vec3( 0.5f,  1.0f, 0.0f), vec2(1.0f, 1.0f) },
            { vec3( 0.5f,  0.0f, 0.0f), vec2(1.0f, 0.0f) },
            { vec3(-0.5f,  0.0f, 0.0f), vec2(0.0f, 0.0f) },
            { vec3(-0.5f,  1.0f, 0.0f), vec2(0.0f, 1.0f) },
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, player.ibo);
        u32 indices[6] = { 0, 2, 1, 2, 0, 3 };  
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, vertices->pos.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, vertices->tex.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)(sizeof(vertices->pos)));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
       
    u32 cube_vao;
    u32 cube_vbo;
    u32 cube_ibo;
    
    {   // Create cube.
        glGenVertexArrays(1, &cube_vao);
        glGenBuffers(1, &cube_vbo);
        glGenBuffers(1, &cube_ibo);
    
        glBindVertexArray(cube_vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
        Vertex_Pos_Tex vertices[36] = {
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
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ibo);
        u32 indices[36] = {
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, vertices->pos.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, vertices->tex.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)(sizeof(vertices->pos)));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
       
    u32 skybox_vao;
    u32 skybox_vbo;
    u32 skybox_ibo;

    {   // Create skybox.
        glGenVertexArrays(1, &skybox_vao);
        glGenBuffers(1, &skybox_vbo);
        glGenBuffers(1, &skybox_ibo);
    
        glBindVertexArray(skybox_vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
        Vertex_Pos_Tex vertices[4] = {
            { vec3( 1.0f,  1.0f, 0.0f), vec2(1.0f, 1.0f) },
            { vec3( 1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f) },
            { vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f) },
            { vec3(-1.0f,  1.0f, 0.0f), vec2(0.0f, 1.0f) },
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox_ibo);
        u32 indices[6] = { 0, 2, 1, 2, 0, 3 };  
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, vertices->pos.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, vertices->tex.dimension(), GL_FLOAT, GL_FALSE, sizeof(*vertices), (void*)(sizeof(vertices->pos)));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    f32 dt = 0.0f;
    s64 begin_counter = performance_counter();
    const f32 frequency = (f32)performance_frequency();
    
    while (alive(window))
    {   
        poll_events(window);
        tick(world, dt);

        // @Cleanup: temp listener position update here.
        alListener3f(AL_POSITION, player.location.x, player.location.y, player.location.z);

        check_shader_hot_reload_queue(&shader_hot_reload_queue);
        
        glClearColor(0.9f, 0.4f, 0.5f, 1.0f); // ugly bright pink
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        const Camera* current_camera = desired_camera(world);

#if 1
        {   // Draw skybox.
            glDepthMask(GL_FALSE);

            glUseProgram(shaders.skybox.id);
            glBindVertexArray(skybox_vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox_ibo);
            glBindTexture(GL_TEXTURE_2D, (GLuint)textures.skybox.id);

            glActiveTexture(GL_TEXTURE0);

            const u32 id = shaders.skybox.id;
            const vec2 scale = vec2(8.0f, 3.0f);
            const vec3 offset = camera.eye;
            glUniform2f(glGetUniformLocation(id, "u_scale"), scale.x, scale.y);
            glUniform3f(glGetUniformLocation(id, "u_offset"), offset.x, offset.y, offset.z);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);

            glDepthMask(GL_TRUE);
        }
#endif
        
        {   // Draw player.
            glUseProgram(shaders.player.id);
            glBindVertexArray(player.vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, player.ibo);
            glBindTexture(GL_TEXTURE_2D, player.texture_id);

            glActiveTexture(GL_TEXTURE0);

            const mat4 m = mat4_transform(player.location, player.rotation, player.scale);
            const mat4 v = camera_view(current_camera);
            const mat4 p = camera_projection(current_camera);
            const mat4 mvp = m * v * p;
            
            glUniformMatrix4fv(glGetUniformLocation(shaders.player.id, "u_mvp"), 1, GL_FALSE, mvp.ptr());             
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }

        {   // Draw cube.
            glUseProgram(shaders.pos_tex.id);
            glBindVertexArray(cube_vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ibo);
            glBindTexture(GL_TEXTURE_2D, textures.stone.id);

            glActiveTexture(GL_TEXTURE0);

            const mat4 m = mat4_transform(vec3(3.0f, 0.5f, 4.0f), quat(), vec3(1.0f));
            const mat4 v = camera_view(current_camera);
            const mat4 p = camera_projection(current_camera);
            const mat4 mvp = m * v * p;
            
            glUniformMatrix4fv(glGetUniformLocation(shaders.pos_tex.id, "u_mvp"), 1, GL_FALSE, mvp.ptr());             
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }
        
        static char text[256];
        const vec3 text_color = vec3(1.0f);
        const f32 padding = atlas->font_size * 0.5f;
        s32 text_size = 0;
        f32 x, y;

        glDepthMask(GL_FALSE); // ignore depth test for debug text
        
        {   // Entity data.
            text_size = (s32)sprintf_s(text, sizeof(text), "player location %s velocity %s", to_string(player.location), to_string(player.velocity));
            x = padding;
            y = (f32)viewport.height - atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "camera eye %s at %s", to_string(current_camera->eye), to_string(current_camera->at));
            x = padding;
            y -= atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }
        
        {   // Runtime stats.
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d %s", dt * 1000.0f, 1 / dt, window->width, window->height, build_type_name);
            x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y = viewport.height - (f32)atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

        {   // Controls.
            text_size = (s32)sprintf_s(text, sizeof(text), "F1 %s F2 %s F3 %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
            x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y = padding;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "Shift/Control + Arrows - force move/rotate game camera");
            x = viewport.width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

        
        {   // Memory stats.
            u64 persistent_size;
            u64 persistent_used;
            usage_persistent(&persistent_size, &persistent_used);
            f32 persistent_part = (f32)persistent_used / persistent_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)persistent_used / 1024 / 1024, (f32)persistent_size / 1024 / 1024, persistent_part);
            x = padding;
            y = padding;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
            
            u64 frame_size;
            u64 frame_used;
            usage_frame(&frame_size, &frame_used);
            f32 frame_part = (f32)frame_used / frame_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame_used / 1024 / 1024, (f32)frame_size / 1024 / 1024, frame_part);
            x = padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
            
            u64 temp_size;
            u64 temp_used;
            usage_temp(&temp_size, &temp_used);
            f32 temp_part = (f32)temp_used / temp_size * 100.0f;
            
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp_used / 1024 / 1024, (f32)temp_size / 1024 / 1024, temp_part);
            x = padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

        glDepthMask(GL_TRUE);
                
        gl_swap_buffers(window);
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
