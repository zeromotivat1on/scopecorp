#include "pch.h"
#include "memory.h"
#include "arena.h"
#include "input.h"
#include "window.h"
#include "my_time.h"
#include "render/gl.h"
#include "render/shader.h"
#include "render/vertex.h"
#include "render/texture.h"
#include "audio/al.h"
#include "audio/alc.h"
#include "font.h"
#include "file.h"
#include "thread.h"
#include "profile.h"
#include "game/world.h"
#include "game/game.h"
#include "editor/hot_reload.h"
#include <stdio.h>

static void on_window_event(Window* window, Window_Event* event)
{
    if (event->type == EVENT_RESIZE)
    {
        const s16 window_w = window->width;
        const s16 window_h = window->height;

        world->camera.aspect = (f32)window_w / window_h;
        world->camera.left = 0.0f;
        world->camera.right = (f32)window_w;
        world->camera.bottom = 0.0f;
        world->camera.top = (f32)window_h;

        world->ed_camera = world->camera;
    
        glViewport(0, 0, window_w, window_h);
        on_framebuffer_resize(font_render_ctx, window_w, window_h);
    }
    
    handle_event(window, event);
}

int main()
{
    prealloc_root();
    prealloc_persistent(MB(64));
    prealloc_frame(MB(16));
    prealloc_temp(MB(64));
    
    init_input_table();
    
    window = create_window(1280, 720, "Scopecorp", 32, 32);
    if (!window)
    {
        log("Failed to create window");
        return 1;
    }

    register_event_callback(window, on_window_event);
    
    gl_init(window, 4, 6);
    gl_vsync(true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);  
    glCullFace(GL_BACK);  
    glFrontFace(GL_CCW);

    ALCdevice* audio_device = alcOpenDevice(null);
    if (!audio_device)
    {
        log("Failed to open default audio device");
        return 1;
    }

    ALCcontext* audio_context = alcCreateContext(audio_device, null);
    if (!audio_context)
    {
        log("Failed to create alc context");
        return 1;
    }

    if (!alcMakeContextCurrent(audio_context))
    {
        log("Failed to make alc context current");
        return 1;
    }

    s32 channel_count;
    s32 sample_rate;
    s32 bit_depth;
    char* sound_data;
    ALsizei sound_data_size;
    
    {
        SCOPE_TIMER("wave file load");

        u64 file_size;
        sound_data = (char*)read_entire_file_temp(DIR_SOUNDS "2814_Recovery.wav", &file_size);
        log("%llu", file_size);
        
        sound_data = (char*)extract_wav(sound_data, &channel_count, &sample_rate, &bit_depth, &sound_data_size);
        //sound_data = load_wav(DIR_SOUNDS "2814_Recovery.wav", channel_count, sample_rate, bit_depth, sound_data_size);

        if (!sound_data)
        {
            log("Could not load wave file");
            return 1;
        }
        
    }
 
    u32 audio_buffer;
    alGenBuffers(1, &audio_buffer);

    ALenum audio_format;
    if (channel_count == 1 && bit_depth == 8)       audio_format = AL_FORMAT_MONO8;
    else if (channel_count == 1 && bit_depth == 16) audio_format = AL_FORMAT_MONO16;
    else if (channel_count == 2 && bit_depth == 8)  audio_format = AL_FORMAT_STEREO8;
    else if (channel_count == 2 && bit_depth == 16) audio_format = AL_FORMAT_STEREO16;
    else
    {
        log("Unknown wave format");
        return 1;
    }

    ALenum al_error;
    // @Cleanup: loading whole data is bad if its size is big, stream in such case.
    alBufferData(audio_buffer, audio_format, sound_data, sound_data_size, sample_rate);

    if ((al_error = alGetError()) != AL_NO_ERROR) log("al_error (0x%X)", al_error);

    alListener3f(AL_POSITION, 0, 0, 0);
    
    ALuint source;
    alGenSources(1, &source);
    alSourcef(source, AL_PITCH, 1);
    alSourcef(source, AL_GAIN, 0.02f);
    alSource3f(source, AL_POSITION, 0, 0, 0);
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    alSourcei(source, AL_LOOPING, AL_FALSE);
    alSourcei(source, AL_BUFFER, audio_buffer);

    if ((al_error = alGetError()) != AL_NO_ERROR) log("al_error (0x%X)", al_error);
    
    alSourcePlay(source);
    log("Started playing wave sound");
    
    /* 
    ALint state = AL_PLAYING;
    while (state == AL_PLAYING)
    {
        alGetSourcei(source, AL_SOURCE_STATE, &state);
    }
    */
    
    compile_game_shaders(&shaders);
    load_game_textures(&textures);
    
    Hot_Reload_List hot_reload_list = {0};
    register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);

    font_render_ctx = create_font_render_context(window->width, window->height);
    Font* font = create_font("C:/Windows/Fonts/Consola.ttf");
    Font_Atlas* atlas = bake_font_atlas(font, 0, 128, 16);

    world = create_world();
    
    const f32 player_scale_aspect = (f32)textures.player.width / textures.player.height;
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
	camera.aspect = (f32)window->width / window->height;
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
        
        check_shader_to_hot_reload();
        
        glClearColor(0.9f, 0.4f, 0.5f, 1.0f); // ugly bright pink
        glClear(GL_COLOR_BUFFER_BIT);

        const Camera* current_camera = desired_camera(world);

        {   // Draw skybox.
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
        }

        {   // Draw player.
            glUseProgram(shaders.player.id);
            glBindVertexArray(player.vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, player.ibo);
            glBindTexture(GL_TEXTURE_2D, (GLuint)textures.player.id);

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
        
        static char text[256];
        const vec3 text_color = vec3(1.0f);
        const f32 padding = atlas->font_size * 0.5f;
        s32 text_size = 0;
        f32 x, y;

        {   // Entity data.
            text_size = (s32)sprintf_s(text, sizeof(text), "player location %s velocity %s", to_string(player.location), to_string(player.velocity));
            x = padding;
            y = (f32)window->height - atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "camera eye %s at %s", to_string(current_camera->eye), to_string(current_camera->at));
            x = padding;
            y -= atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }
        
        {   // Runtime stats.
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d %s", dt * 1000.0f, 1 / dt, window->width, window->height, build_type_name);
            x = window->width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y = window->height - (f32)atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

        {   // Controls.
            text_size = (s32)sprintf_s(text, sizeof(text), "F1 %s F2 %s F3 %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
            x = window->width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y = padding;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "Shift/Control + Arrows - force move/rotate game camera");
            x = window->width - line_width_px(atlas, text, (s32)strlen(text)) - padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

        
        {   // Memory stats.
            u64 persistent_size;
            u64 persistent_used;
            usage_persistent(&persistent_size, &persistent_used);
            f32 persistent_part = (f32)persistent_used / persistent_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Persistent)", (f32)persistent_used / 1024 / 1024, (f32)persistent_size / 1024 / 1024, (f32)persistent_part / 1024 / 1024);
            x = padding;
            y = padding;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
            
            u64 frame_size;
            u64 frame_used;
            usage_frame(&frame_size, &frame_used);
            f32 frame_part = (f32)frame_used / frame_size * 100.0f;

            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Frame)", (f32)frame_used / 1024 / 1024, (f32)frame_size / 1024 / 1024, (f32)frame_part / 1024 / 1024);
            x = padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
            
            u64 temp_size;
            u64 temp_used;
            usage_temp(&temp_size, &temp_used);
            f32 temp_part = (f32)temp_used / temp_size * 100.0f;
            
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fmb/%.2fmb (%.2f%% | Temp)", (f32)temp_used / 1024 / 1024, (f32)temp_size / 1024 / 1024, (f32)temp_part / 1024 / 1024);
            x = padding;
            y += atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }

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

    destroy(window);
    free_root();
    
    return 0;
}
