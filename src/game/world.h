#pragma once

#include "camera.h"
#include "sparse_array.h"
#include "collision.h"
#include "game/entity.h"

#define WORLD_LEVEL_EXTENSION_NAME ".wl"

inline constexpr s32 MAX_WORLD_NAME_SIZE = 64;

inline constexpr s32 MAX_STATIC_MESHES = 1024;
inline constexpr s32 MAX_DIRECT_LIGHTS = 4;
inline constexpr s32 MAX_POINT_LIGHTS  = 32;
inline constexpr s32 MAX_AABBS = 2048;

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

	Sparse_Array<Static_Mesh>  static_meshes;
	Sparse_Array<Point_Light>  point_lights;
	Sparse_Array<Direct_Light> direct_lights;
    
	Sparse_Array<AABB> aabbs;

    Entity *mouse_picked_entity = null;
};

inline World *world = null;

void init_world(World *world);
void save_world_level(World *world);
void load_world_level(World *world, const char *path);
void tick(World *world, f32 dt);
Camera *desired_camera(World *world);

s32 create_static_mesh(World *world);

Entity *find_entity_by_id(World* world, s32 id);

void draw_world(const World *world);
void draw_entity(const Entity *e);

void for_each_entity(World *world, For_Each_Entity_Callback callback, void *user_data = null);
