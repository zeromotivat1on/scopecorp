#pragma once

#include "reflection.h"

#include "math/vector.h"
#include "math/quat.h"

// Used during entity creation.
inline eid eid_global_counter = 1;

enum Entity_Type : u8 {
    E_PLAYER,
    E_SKYBOX,
    E_STATIC_MESH,
    E_DIRECT_LIGHT,
    E_POINT_LIGHT,
    E_SOUND_EMITTER_2D,
    E_SOUND_EMITTER_3D,
    E_PORTAL,
    
    E_COUNT
};

enum Entity_Bits : u32 {
    E_MOUSE_PICKED_BIT = 0x1,
};

struct Entity_Draw_Data {
    sid sid_mesh     = SID_NONE;
    sid sid_material = SID_NONE;
    
    u32 eid_offset = 0;
};

struct Entity {
    eid eid = EID_NONE;
    Entity_Type type;
    u32 bits = 0;
    
    vec3 location;
    quat rotation;
    vec3 scale = vec3(1.0f);
    vec2 uv_scale = vec2(1.0f);
    
    s32 aabb_index = INDEX_NONE;
    
    Entity_Draw_Data draw_data;
};

#define REFLECT_ENTITY_FIELDS                                           \
    REFLECT_FIELD(Entity, eid,        FIELD_U32)                        \
    REFLECT_FIELD(Entity, type,       FIELD_U8)                         \
    REFLECT_FIELD(Entity, bits,       FIELD_U32)                        \
    REFLECT_FIELD(Entity, location,   FIELD_VEC3)                       \
    REFLECT_FIELD(Entity, rotation,   FIELD_QUAT)                       \
    REFLECT_FIELD(Entity, scale,      FIELD_VEC3)                       \
    REFLECT_FIELD(Entity, uv_scale,   FIELD_VEC2)                       \
    REFLECT_FIELD(Entity, aabb_index, FIELD_S32)                        \
    REFLECT_FIELD(Entity, draw_data.sid_mesh,     FIELD_SID)            \
    REFLECT_FIELD(Entity, draw_data.sid_material, FIELD_SID)            \
    REFLECT_FIELD(Entity, draw_data.eid_offset,   FIELD_U32)            \

REFLECT_BEGIN(Entity)
REFLECT_ENTITY_FIELDS
REFLECT_END(Entity)

struct Player : Entity {
    Player() { type = E_PLAYER; }
    
    f32 move_speed      = 3.0f;
    f32 ed_camera_speed = 4.0f;
    
    f32 mouse_sensitivity = 16.0f;
    
    vec3 camera_offset       = vec3(0.0f, 1.0f, -3.0f);
    vec3 camera_dead_zone    = vec3(1.0f, 1.0f, 1.0f);
    f32  camera_follow_speed = 16.0f;
    
    vec3 velocity            = vec3_zero;
    Direction move_direction = SOUTH;

    sid sid_flip_book_move = SID_NONE;
    sid sid_sound_steps = 0;
    
    s32 collide_aabb_index = INDEX_NONE;
};

REFLECT_BEGIN(Player)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Player, move_speed, FIELD_F32)
REFLECT_FIELD(Player, ed_camera_speed, FIELD_F32)
REFLECT_FIELD(Player, mouse_sensitivity, FIELD_F32)
REFLECT_FIELD(Player, camera_offset, FIELD_VEC3)
REFLECT_FIELD(Player, camera_dead_zone, FIELD_VEC3)
REFLECT_FIELD(Player, camera_follow_speed, FIELD_F32)
REFLECT_FIELD(Player, velocity, FIELD_VEC3)
REFLECT_FIELD(Player, move_direction, FIELD_U8)
REFLECT_FIELD(Player, sid_flip_book_move, FIELD_SID)
REFLECT_FIELD(Player, sid_sound_steps, FIELD_SID)
REFLECT_FIELD(Player, collide_aabb_index, FIELD_S32)
REFLECT_END(Player)

struct Static_Mesh : Entity {
    Static_Mesh() { type = E_STATIC_MESH; }
};

REFLECT_BEGIN(Static_Mesh)
REFLECT_ENTITY_FIELDS
REFLECT_END(Static_Mesh)

struct Skybox : Entity {
    Skybox() { type = E_SKYBOX; }

    vec3 uv_offset = vec3_zero;
};

REFLECT_BEGIN(Skybox)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Skybox, uv_offset, FIELD_VEC2)
REFLECT_END(Skybox)

struct Direct_Light : Entity {
    Direct_Light() { type = E_DIRECT_LIGHT; }

    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_white;
};

REFLECT_BEGIN(Direct_Light)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Direct_Light, ambient,  FIELD_VEC3)
REFLECT_FIELD(Direct_Light, diffuse,  FIELD_VEC3)
REFLECT_FIELD(Direct_Light, specular, FIELD_VEC3)
REFLECT_END(Direct_Light)

struct Point_Light : Entity {
    Point_Light() { type = E_POINT_LIGHT; }
    
    vec3 ambient  = vec3_white;
    vec3 diffuse  = vec3_white;
    vec3 specular = vec3_white;
    
    f32 attenuation_constant  = 0.0f;
    f32 attenuation_linear    = 0.0f;
    f32 attenuation_quadratic = 0.0f;
};

REFLECT_BEGIN(Point_Light)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Point_Light, ambient,  FIELD_VEC3)
REFLECT_FIELD(Point_Light, diffuse,  FIELD_VEC3)
REFLECT_FIELD(Point_Light, specular, FIELD_VEC3)
REFLECT_FIELD(Point_Light, attenuation_constant,  FIELD_F32)
REFLECT_FIELD(Point_Light, attenuation_linear,    FIELD_F32)
REFLECT_FIELD(Point_Light, attenuation_quadratic, FIELD_F32)
REFLECT_END(Point_Light)

struct Sound_Emitter_2D : Entity {
    Sound_Emitter_2D() { type = E_SOUND_EMITTER_2D; }

    sid sid_sound = SID_NONE;
};

REFLECT_BEGIN(Sound_Emitter_2D)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Sound_Emitter_2D, sid_sound, FIELD_SID)
REFLECT_END(Sound_Emitter_2D)

struct Sound_Emitter_3D : Entity {
    Sound_Emitter_3D() { type = E_SOUND_EMITTER_3D; }

    sid sid_sound = SID_NONE;
};

REFLECT_BEGIN(Sound_Emitter_3D)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Sound_Emitter_3D, sid_sound, FIELD_SID)
REFLECT_END(Sound_Emitter_3D)

struct Portal : Entity {
    Portal() { type = E_PORTAL; }

    vec3 destination_location = vec3_zero;
};

REFLECT_BEGIN(Portal)
REFLECT_ENTITY_FIELDS
REFLECT_FIELD(Portal, destination_location, FIELD_VEC3)
REFLECT_END(Portal)

inline constexpr String Entity_type_names[E_COUNT] = {
    S("Player"),
    S("Skybox"),
    S("Static Mesh"),
    S("Direct Light"),
    S("Point Light"),
    S("Sound Emitter 2D"),
    S("Sound Emitter 3D"),
    S("Portal"),
};

inline u32 entity_type_field_counts[E_COUNT] = {
    REFLECT_FIELD_COUNT(Player),
    REFLECT_FIELD_COUNT(Skybox),
    REFLECT_FIELD_COUNT(Static_Mesh),
    REFLECT_FIELD_COUNT(Direct_Light),
    REFLECT_FIELD_COUNT(Point_Light),
    REFLECT_FIELD_COUNT(Sound_Emitter_2D),
    REFLECT_FIELD_COUNT(Sound_Emitter_3D),
    REFLECT_FIELD_COUNT(Portal),
};

const Reflect_Field &get_entity_field(Entity_Type type, u32 index);
