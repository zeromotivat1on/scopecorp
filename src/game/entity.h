#pragma once

#include "math/vector.h"
#include "math/quat.h"

typedef u64 sid;

inline constexpr s32 INVALID_ENTITY_ID = -1;

enum Entity_Type {
    ENTITY_PLAYER,
    ENTITY_SKYBOX,
    ENTITY_STATIC_MESH,
    ENTITY_DIRECT_LIGHT,
    ENTITY_POINT_LIGHT,
};

enum Entity_FLag : u32 {
    ENTITY_FLAG_SELECTED_IN_EDITOR = 0x1,
};

struct Entity_Draw_Data {
    s32 vertex_array_index = INVALID_INDEX;
    s32 index_buffer_index = INVALID_INDEX;
    s32 material_index     = INVALID_INDEX;
};

struct Entity {
    u32 flags = 0;
    Entity_Type type;
    s32 id = INVALID_ENTITY_ID;
    
    vec3 location;
    quat rotation;
    vec3 scale = vec3(1.0f);

    s32 aabb_index = INVALID_INDEX;
    
    Entity_Draw_Data draw_data;
};

struct Flip_Book;

struct Player : Entity {
    Player() { type = ENTITY_PLAYER; }
    
    f32 move_speed      = 3.0f;
    f32 ed_camera_speed = 4.0f;
    
    f32 mouse_sensitivity = 32.0f;
    
    vec3 camera_offset       = vec3(0.0f, 1.0f, -3.0f);
    vec3 camera_dead_zone    = vec3(1.0f, 1.0f, 1.0f);
    f32  camera_follow_speed = 16.0f;
    
    vec3 velocity            = vec3_zero;
    Direction move_direction = DIRECTION_BACK;
    
    Flip_Book *flip_book = null;
    sid steps_sid = 0;
    
    s32 collide_aabb_index = INVALID_INDEX;
};

struct Static_Mesh : Entity {
    Static_Mesh() { type = ENTITY_STATIC_MESH; }
};

struct Skybox : Entity {
    Skybox() { type = ENTITY_SKYBOX; }

    vec2 uv_scale;
    vec3 uv_offset;
};

struct Direct_Light : Entity {
    Direct_Light() { type = ENTITY_DIRECT_LIGHT; }

    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_white;

    s32 u_light_index = INVALID_INDEX; // index in Direct_Lights uniform block
};

struct Light_Attenuation {
    f32 constant  = 0.0f;
    f32 linear    = 0.0f;
    f32 quadratic = 0.0f;
};

struct Point_Light : Entity {
    Point_Light() { type = ENTITY_POINT_LIGHT; }
    
    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_white;

    Light_Attenuation attenuation;
    
    s32 u_light_index = INVALID_INDEX; // index in Point_Lights uniform block
};
