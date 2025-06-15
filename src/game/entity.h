#pragma once

#include "math/vector.h"
#include "math/quat.h"

enum Entity_Type {
    ENTITY_PLAYER,
    ENTITY_SKYBOX,
    ENTITY_STATIC_MESH,
    ENTITY_DIRECT_LIGHT,
    ENTITY_POINT_LIGHT,
    ENTITY_SOUND_EMITTER_2D,
    ENTITY_SOUND_EMITTER_3D,
    ENTITY_PORTAL,
};

enum Entity_FLag : u32 {
    ENTITY_FLAG_SELECTED_IN_EDITOR = 0x1,
};

struct Entity_Draw_Data {
    sid sid_mesh     = SID_NONE;
    sid sid_material = SID_NONE;

    u32 eid_vertex_data_offset = 0;
};

struct Entity {
    u32 flags = 0;
    Entity_Type type;
    eid eid = EID_NONE;
    
    vec3 location;
    quat rotation;
    vec3 scale = vec3(1.0f);

    s32 aabb_index = INVALID_INDEX;
    
    Entity_Draw_Data draw_data;
};

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

    sid sid_flip_book_move = SID_NONE;
    sid sid_sound_steps = 0;
    
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

struct Sound_Emitter_2D : Entity {
    Sound_Emitter_2D() { type = ENTITY_SOUND_EMITTER_2D; }

    sid sid_sound = SID_NONE;
};

struct Sound_Emitter_3D : Entity {
    Sound_Emitter_3D() { type = ENTITY_SOUND_EMITTER_3D; }

    sid sid_sound = SID_NONE;
};

struct Portal : Entity {
    Portal() { type = ENTITY_PORTAL; }

    vec3 destination_location = vec3_zero;
};
