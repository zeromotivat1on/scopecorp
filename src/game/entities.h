#pragma once

#include "math/vector.h"
#include "math/quat.h"

#include "collision.h"

inline constexpr s32 INVALID_ENTITY_ID = -1;

enum Entity_Type {
    E_UNKNOWN,
    E_PLAYER,
    E_SKYBOX,
    E_STATIC_MESH
};

struct Entity_Draw_Data {
    u32 flags = 0;
    s32 vbi   = INVALID_INDEX;
    s32 ibi   = INVALID_INDEX;
    s32 mti   = INVALID_INDEX;
};

struct Entity {
    Entity_Type type = E_UNKNOWN;
    s32 id = INVALID_ENTITY_ID;
    
    vec3 location;
    quat rotation;
    vec3 scale = vec3(1.0f);

    mat4 mvp;

    Entity_Draw_Data draw_data;
};

struct Flip_Book;

struct Player : Entity {
    Player() { type = E_PLAYER; }
    
    f32 move_speed      = 3.0f;
    f32 ed_camera_speed = 4.0f;
    
    f32 mouse_sensitivity = 32.0f;
    
    vec3 camera_offset       = vec3(0.0f, 1.0f, -3.0f);
    vec3 camera_dead_zone    = vec3(1.0f, 1.0f, 1.0f);
    f32  camera_follow_speed = 16.0f;
    
    vec3 velocity            = vec3_zero;
    Direction move_direction = DIRECTION_BACK;
    
    Flip_Book *flip_book = null;

    AABB aabb;

    s32 collide_mesh_index = INVALID_INDEX;
};

struct Static_Mesh : Entity {
    Static_Mesh() { type = E_STATIC_MESH; }
    
    AABB aabb;
};

struct Skybox : Entity {
    Skybox() { type = E_SKYBOX; }

    vec2 uv_scale = vec2(8.0f, 4.0f);
    vec3 uv_offset;
};
