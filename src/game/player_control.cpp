#include "pch.h"
#include "player_control.h"
#include "input.h"
#include "game.h"
#include "world.h"
#include "my_math.h"

void press(s32 key, bool pressed)
{
    
}

void tick(Player* player, f32 dt)
{
    if (input_table.key_states[KEY_SHIFT])
    {
        const f32 speed = player->ed_camera_speed * dt;
        vec3 velocity;

        if (input_table.key_states[KEY_RIGHT])
            velocity.x = speed;
    
        if (input_table.key_states[KEY_LEFT])
            velocity.x = -speed;
    
        if (input_table.key_states[KEY_UP])
            velocity.y = speed;
    
        if (input_table.key_states[KEY_DOWN])
            velocity.y = -speed;

        Camera& camera = world->camera;
        camera.eye += velocity.truncate(speed);
        camera.at = camera.eye + vec3_forward(camera.yaw, camera.pitch);
    }
    else if (input_table.key_states[KEY_CTRL])
    {
        Camera& camera = world->camera;
        const f32 speed = 4.0f * player->mouse_sensitivity * dt;

        if (input_table.key_states[KEY_RIGHT])
            camera.yaw -= speed;
    
        if (input_table.key_states[KEY_LEFT])
            camera.yaw += speed;
    
        if (input_table.key_states[KEY_UP])
            camera.pitch += speed;
    
        if (input_table.key_states[KEY_DOWN])
            camera.pitch -= speed;

        camera.pitch = clamp(camera.pitch, -89.0f, 89.0f);
        camera.at = camera.eye + vec3_forward(camera.yaw, camera.pitch);
    }
    
    if (game_state.mode == MODE_GAME)
    {
        const f32 speed = player->move_speed * dt;
        vec3 velocity;
    
        // @Todo: use input action instead of direct key state.
        if (game_state.player_movement_behavior == INDEPENDENT)
        {
            if (input_table.key_states[KEY_D])
                velocity.x = speed;
    
            if (input_table.key_states[KEY_A])
                velocity.x = -speed;
    
            if (input_table.key_states[KEY_W])
                velocity.z = speed;
    
            if (input_table.key_states[KEY_S])
                velocity.z = -speed;        
        }
        else if (game_state.player_movement_behavior == RELATIVE_TO_CAMERA)
        {
            Camera& camera = world->camera;
            const vec3 camera_forward = vec3_forward(camera.yaw, camera.pitch);
            const vec3 camera_right = camera.up.cross(camera_forward).normalize();

            if (input_table.key_states[KEY_D])
                velocity += speed * camera_right;
    
            if (input_table.key_states[KEY_A])
                velocity -= speed * camera_right;
    
            if (input_table.key_states[KEY_W])
                velocity += speed * camera_forward;
    
            if (input_table.key_states[KEY_S])
                velocity -= speed * camera_forward;
        }
        
        player->velocity = velocity.truncate(speed);
        player->location += player->velocity;

        if (game_state.camera_behavior == STICK_TO_PLAYER)
        {
            Camera& camera = world->camera;
            camera.eye = player->location + player->camera_offset;
            camera.at = camera.eye + vec3_forward(camera.yaw, camera.pitch);
        }
    }
    else if (game_state.mode == MODE_EDITOR)
    {
        const f32 mouse_sensitivity = player->mouse_sensitivity;
        Camera& camera = world->ed_camera;

        camera.yaw   += input_table.mouse_offset_x * mouse_sensitivity * dt;
        camera.pitch += input_table.mouse_offset_y * mouse_sensitivity * dt;
        camera.pitch  = clamp(camera.pitch, -89.0f, 89.0f);
        
        const f32 speed = player->ed_camera_speed * dt;
        const vec3 camera_forward = vec3_forward(camera.yaw, camera.pitch);
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
