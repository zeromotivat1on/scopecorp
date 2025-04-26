#pragma once

#include "camera.h"
#include "sparse_array.h"
#include "collision.h"
#include "game/entity.h"

inline constexpr s32 MAX_WORLD_NAME_SIZE = 64;
inline constexpr s32 MAX_STATIC_MESHES = 1024;
inline constexpr s32 MAX_AABBS         = MAX_STATIC_MESHES + 1;

struct World {
    char name[MAX_WORLD_NAME_SIZE] = {0};
    
	f32 dt;

	Player player;
	Camera camera;
	Camera ed_camera;

	Skybox skybox;

	Sparse_Array<Static_Mesh> static_meshes;
	Sparse_Array<AABB>        aabbs;

    Entity *mouse_picked_entity = null;
    
    mat4 camera_view;
    mat4 camera_proj;
    mat4 camera_view_proj;
};

inline World *world = null;

void init_world(World *world);
void tick(World *world, f32 dt);
Camera *desired_camera(World *world);
Entity *find_entity_by_id(World* world, s32 id);
s32 create_static_mesh(World *world);

void draw_world(const World *world);
void draw_entity(const Entity *e);
