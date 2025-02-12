#include "pch.h"
#include "memory.h"
#include "arena.h"
#include "input.h"
#include "window.h"
#include "time.h"
#include "render/gl.h"
#include "render/shader.h"
#include "render/texture.h"
#include "font.h"
#include "thread.h"
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
    prealloc_frame(MB(4));
    prealloc_temp(MB(2));
    
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

    compile_game_shaders(&shaders);
    load_game_textures(&textures);
    
    Hot_Reload_List hot_reload_list = {0};
    register_hot_reload_dir(&hot_reload_list, DIR_SHADERS, on_shader_changed_externally);

    font_render_ctx = create_font_render_context(window->width, window->height);
    Font* font = create_font("C:/Windows/Fonts/Consola.ttf");
    Font_Atlas* atlas = bake_font_atlas(font, 0, 128, 16);

    world = create_world();

    Player& player = world->player;
    player.location = vec3(0.0f, 0.0f, 0.0f);
    player.camera_offset = vec3(0.0f, 1.0f, -3.0f);
    
    const f32 player_scale_aspect = (f32)textures.player.width / textures.player.height;
    const f32 player_y_scale = 1.0f * player_scale_aspect;
    const f32 player_x_scale = player_y_scale * player_scale_aspect;
    player.scale = vec3(player_x_scale, player_y_scale, 1.0f);
    
    Camera& camera = world->camera;
    camera.mode = MODE_PERSPECTIVE;
    camera.yaw = 90.0f;
	camera.pitch = 0.0f;
	camera.eye = player.camera_offset;
	camera.at = camera.eye + vec3_forward(camera.yaw, camera.pitch);
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
    
    {
        glGenVertexArrays(1, &player.vao);
        glGenBuffers(1, &player.vbo);
    
        glBindVertexArray(player.vao);
        glBindBuffer(GL_ARRAY_BUFFER, player.vbo);

        f32 vertices[4 * 3] = {
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
        };
    
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void*)0);
    
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    f32 dt = 0.0f;
    s64 begin_counter = performance_counter();
    const f32 frequency = (f32)performance_frequency();
    
    while (alive(window))
    {   
        poll_events(window);
        tick(world, dt);

        check_shader_to_hot_reload();
        
        glClearColor(0.0f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const Camera* current_camera = desired_camera(world);
        
        {
            glUseProgram(shaders.player.id);
            glBindVertexArray(player.vao);
            glBindBuffer(GL_ARRAY_BUFFER, player.vbo);
            glBindTexture(GL_TEXTURE_2D, (GLuint)textures.player.id);

            glActiveTexture(GL_TEXTURE0);

            const mat4 m = mat4_transform(player.location, player.rotation, player.scale);
            const mat4 v = camera_view(current_camera);
            const mat4 p = camera_projection(current_camera);
            const mat4 mvp = m * v * p;
            
            glUniformMatrix4fv(glGetUniformLocation(shaders.player.id, "u_mvp"), 1, GL_FALSE, mvp.ptr());             
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }

        static char text[256];
        const vec3 text_color = vec3(1.0f);
        const f32 hor_padding = atlas->font_size * 0.5f;
        s32 text_size = 0;
        f32 x, y;

        {
            text_size = (s32)sprintf_s(text, sizeof(text), "player location %s velocity %s", to_string(player.location), to_string(player.velocity));
            x = hor_padding;
            y = (f32)window->height - atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "camera eye %s at %s", to_string(current_camera->eye), to_string(current_camera->at));
            x = hor_padding;
            y -= atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }
        
        {
            text_size = (s32)sprintf_s(text, sizeof(text), "%.2fms %.ffps %dx%d", dt * 1000.0f, 1 / dt, window->width, window->height);
            x = window->width - line_width_px(atlas, text, (s32)strlen(text)) - hor_padding;
            y = window->height - (f32)atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);

            text_size = (s32)sprintf_s(text, sizeof(text), "%s %s %s", to_string(game_state.mode), to_string(game_state.camera_behavior), to_string(game_state.player_movement_behavior));
            x = window->width - line_width_px(atlas, text, (s32)strlen(text)) - hor_padding;
            y -= atlas->font_size;
            render_text(font_render_ctx, atlas, text, text_size, vec2(x, y), text_color);
        }
        
        gl_swap_buffers(window);
        free_all_frame();
        
        const s64 end_counter = performance_counter();
        dt = (end_counter - begin_counter) / frequency;
		begin_counter = end_counter;
    }

    destroy(window);
    free_root();
    
    return 0;
}
