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
    f32 mouse_sensitivity = 32.0f;
    
    vec3 camera_offset = vec3(0.0f, 1.0f, -3.0f);
    vec3 camera_dead_zone = vec3(1.0f, 1.0f, 8.0f);
    f32 camera_follow_speed = 16.0f;
    
    vec3 velocity;
    Direction move_direction;
    
    struct Flip_Book* flip_book; // desired flip book to use
    u32 texture_id; // desired texture to draw
    
    u32 vao;
    u32 vbo;
    u32 ibo;
};
