#pragma once

#include "camera.h"
#include "sparse_array.h"
#include "collision.h"
#include "game/entity.h"

inline constexpr u32 MAX_WORLD_NAME_SIZE = 64;

inline constexpr u32 MAX_STATIC_MESHES     = 1024;
inline constexpr u32 MAX_DIRECT_LIGHTS     = 4;
inline constexpr u32 MAX_POINT_LIGHTS      = 32;
inline constexpr u32 MAX_SOUND_EMITTERS_2D = 32;
inline constexpr u32 MAX_SOUND_EMITTERS_3D = 128;
inline constexpr u32 MAX_PORTALS           = 128;
inline constexpr u32 MAX_AABBS = 2048;

enum For_Each_Result {
    RESULT_CONTINUE,
    RESULT_BREAK,
};

typedef For_Each_Result (* For_Each_Entity_Callback)(Entity *e, void *user_data);

struct World {
    char name[MAX_WORLD_NAME_SIZE];

	f32 dt;

	Player player;
	Camera camera;
	Camera ed_camera;

	Skybox skybox;

	Sparse_Array<Static_Mesh>      static_meshes;
	Sparse_Array<Point_Light>      point_lights;
	Sparse_Array<Direct_Light>     direct_lights;
	Sparse_Array<Sound_Emitter_2D> sound_emitters_2d;
	Sparse_Array<Sound_Emitter_3D> sound_emitters_3d;
	Sparse_Array<Portal>           portals;
    
	Sparse_Array<AABB> aabbs;

    Entity *mouse_picked_entity = null;
};

inline World *world = null;

void init_world(World *world);
void save_level(World *world);
void load_level(World *world, const char *path);
void tick(World *world, f32 dt);
Camera *desired_camera(World *world);

Entity *create_entity(World *world, Entity_Type e_type);
Entity *find_entity_by_eid(World* world, eid eid);

void draw_world(const World *world);
void draw_entity(const Entity *e);

void for_each_entity(World *world, For_Each_Entity_Callback callback, void *user_data = null);
