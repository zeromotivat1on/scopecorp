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

// Reflection does not support enums for now, so their underlying types are used instead.
struct Entity {
    // Meta.
    u8          type;
    u32         bits;
    Pid         eid; // @no_serialize
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
    Matrix4     object_to_world; // @no_serialize
    Vector2     uv_scale;
    Vector3     uv_offset;
    Vector3     velocity;
    AABB        aabb;
    // Player.
    f32         move_speed;
    u8          move_direction;
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
    // Portal.
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
