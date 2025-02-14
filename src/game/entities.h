#pragma once

#include "vector.h"
#include "quat.h"

inline constexpr s32 INVALID_ENTITY_ID = -1;

enum Entity_Type
{
    E_UNKNOWN,
    E_PLAYER
};

struct Entity
{
    Entity_Type type = E_UNKNOWN;
    s32 id = INVALID_ENTITY_ID;
    
    vec3 location;
    quat rotation;
    vec3 scale = vec3(1.0f);
};

struct Player : Entity
{
    Player() { type = E_PLAYER; }
    
    f32 move_speed = 3.0f;
    f32 ed_camera_speed = 4.0f;
    f32 mouse_sensitivity = 16.0f;
    
    vec3 camera_offset = vec3(0.0f, 1.0f, -3.0f);
    vec3 camera_dead_zone = vec3(2.0f);
    
    vec3 velocity;

    // Shader will be obtained from global shader list.
    // These should not be here as well?
    u32 vao;
    u32 vbo;
    u32 ibo;
};
