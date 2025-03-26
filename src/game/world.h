#pragma once

#include "camera.h"
#include "sparse_array.h"
#include "collision.h"
#include "game/entities.h"

inline constexpr s32 MAX_STATIC_MESHES = 1024;
inline constexpr s32 MAX_AABBS         = MAX_STATIC_MESHES + 1;

struct World {
	f32 dt;

	Player player;
	Camera camera;
	Camera ed_camera;

	Skybox skybox;

	Sparse_Array<Static_Mesh> static_meshes;
	Sparse_Array<AABB>        aabbs;

    s32 selected_aabb_index = INVALID_INDEX;
    s32 selected_entity_id  = INVALID_ENTITY_ID;
};

inline World *world = null;

void init_world(World *world);
void tick(World *world, f32 dt);
Camera *desired_camera(World *world);

s32 create_static_mesh(World *world);

void draw_world(const World *world);
void draw_entity(const Entity *e);
