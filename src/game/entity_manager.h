#pragma once

#include "reflection.h"
#include "collision.h"

inline constexpr u32 EID_INDEX_BITS      = 20;
inline constexpr u32 EID_GENERATION_BITS = 12;
static_assert((EID_INDEX_BITS + EID_GENERATION_BITS) == 8 * sizeof(Pid));

inline constexpr u32 EID_INDEX_MAX      = (1u << EID_INDEX_BITS     ) - 1;
inline constexpr u32 EID_GENERATION_MAX = (1u << EID_GENERATION_BITS) - 1;

inline Pid make_eid(u32 index, u16 generation) {
    Assert(index <= EID_INDEX_MAX);
    Assert(index <= EID_GENERATION_MAX);
    return (generation << EID_INDEX_BITS) | index;
}

inline u32 get_eid_index      (Pid eid) { return eid & EID_INDEX_MAX; }
inline u32 get_eid_generation (Pid eid) { return (eid >> EID_INDEX_BITS) & EID_GENERATION_MAX; }

enum Entity_Type : u8 {
    E_NONE,
    E_PLAYER,
    E_SKYBOX,
    E_STATIC_MESH,
    E_DIRECT_LIGHT,
    E_POINT_LIGHT,
    E_SOUND_EMITTER,
    E_PORTAL,
    
    E_COUNT
};

enum Entity_Bits : u32 {
    E_DELETED_BIT      = 0x1,
    E_MOUSE_PICKED_BIT = 0x2,
    E_INVISIBLE_BIT    = 0x4,
    E_OVERLAP_BIT      = 0x8,
};

struct Entity {
    // Meta.
    Entity_Type type;
    u32         bits;
    Pid         eid;
    Pid         parent;
    Pid         first_child;
    Pid         next_sibling;
    Pid         prev_sibling;
    // Common.
    Atom        mesh;
    Atom        material;
    Vector3     position;
    Vector3     scale;
    Quaternion  orientation;
    Matrix4     object_to_world;
    Vector2     uv_scale;
    Vector3     uv_offset;
    Vector3     velocity;
    AABB        aabb;
    // Player.
    f32         move_speed;
    Direction   move_direction;
    Atom        move_flip_book;
    Atom        move_sound;
    Vector3     camera_offset;
    Vector3     camera_dead_zone;
    f32         camera_follow_speed;
    // Light.
    Vector3     ambient_factor;
    Vector3     diffuse_factor;
    Vector3     specular_factor;
    f32         attenuation_constant;
    f32         attenuation_linear;
    f32         attenuation_quadratic;
    // Sound emitter.
    Atom        sound;
    bool        sound_play_spatial;
    // Portal
    Vector3     portal_destination;

    explicit operator bool() const { return type != E_NONE; }
};

struct Entity_Manager {    
    Camera         camera;
    Pid            player;
    Pid            skybox;
    Array <Entity> entities;
    Array <Pid>    entities_to_delete;
    Array <Pid>    free_entities;
};

inline Array <Entity_Manager> entity_managers;

Entity_Manager *get_entity_manager ();
Entity_Manager *new_entity_manager ();

void    reset              (Entity_Manager *manager);
void    post_frame_cleanup (Entity_Manager *manager);
Pid     new_entity         (Entity_Manager *manager, Entity_Type type);
void    delete_entity      (Entity_Manager *manager, Pid eid);
Entity *get_entity         (Entity_Manager *manager, Pid eid);
Entity *get_player         (Entity_Manager *manager);
Entity *get_skybox         (Entity_Manager *manager);

void move_aabb_along_with_entity (Pid eid);
void move_aabb_along_with_entity (Entity *e);

#define New_Entity(M, E) get_entity(M, new_entity(M, E))
#define For_Entities(A, E) For (A) if (it.type == E)

// @Todo: create separate tool to generate such reflection data.
Begin_Reflection(Entity)
Add_Reflection_Field(Entity, type,                  REFLECTION_FIELD_TYPE_U8,         1)
Add_Reflection_Field(Entity, bits,                  REFLECTION_FIELD_TYPE_U32,        1)
// No need to serialze eid as new entities are created during load.
Add_Reflection_Field(Entity, eid,                   REFLECTION_FIELD_TYPE_PID,        0)
Add_Reflection_Field(Entity, parent,                REFLECTION_FIELD_TYPE_PID,        1)
Add_Reflection_Field(Entity, first_child,           REFLECTION_FIELD_TYPE_PID,        1)
Add_Reflection_Field(Entity, next_sibling,          REFLECTION_FIELD_TYPE_PID,        1)
Add_Reflection_Field(Entity, prev_sibling,          REFLECTION_FIELD_TYPE_PID,        1)
Add_Reflection_Field(Entity, mesh,                  REFLECTION_FIELD_TYPE_ATOM,       1)
Add_Reflection_Field(Entity, material,              REFLECTION_FIELD_TYPE_ATOM,       1)
Add_Reflection_Field(Entity, position,              REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, scale,                 REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, orientation,           REFLECTION_FIELD_TYPE_QUATERNION, 1)
// Object to world transform matrix is inferred from position, scale and orientation.
Add_Reflection_Field(Entity, object_to_world,       REFLECTION_FIELD_TYPE_MATRIX4,    0)
Add_Reflection_Field(Entity, uv_scale,              REFLECTION_FIELD_TYPE_VECTOR2,    1)
Add_Reflection_Field(Entity, uv_offset,             REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, velocity,              REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, aabb,                  REFLECTION_FIELD_TYPE_AABB,       1)
Add_Reflection_Field(Entity, move_speed,            REFLECTION_FIELD_TYPE_F32,        1)
Add_Reflection_Field(Entity, move_direction,        REFLECTION_FIELD_TYPE_U8,         1)
Add_Reflection_Field(Entity, move_flip_book,        REFLECTION_FIELD_TYPE_ATOM,       1)
Add_Reflection_Field(Entity, move_sound,            REFLECTION_FIELD_TYPE_ATOM,       1)
Add_Reflection_Field(Entity, camera_offset,         REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, camera_dead_zone,      REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, camera_follow_speed,   REFLECTION_FIELD_TYPE_F32,        1)
Add_Reflection_Field(Entity, ambient_factor,        REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, diffuse_factor,        REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, specular_factor,       REFLECTION_FIELD_TYPE_VECTOR3,    1)
Add_Reflection_Field(Entity, attenuation_constant,  REFLECTION_FIELD_TYPE_F32,        1)
Add_Reflection_Field(Entity, attenuation_linear,    REFLECTION_FIELD_TYPE_F32,        1)
Add_Reflection_Field(Entity, attenuation_quadratic, REFLECTION_FIELD_TYPE_F32,        1)
Add_Reflection_Field(Entity, sound,                 REFLECTION_FIELD_TYPE_ATOM,       1)
Add_Reflection_Field(Entity, sound_play_spatial,    REFLECTION_FIELD_TYPE_BOOL,       1)
Add_Reflection_Field(Entity, portal_destination,    REFLECTION_FIELD_TYPE_VECTOR3,    1)
End_Reflection(Entity)
