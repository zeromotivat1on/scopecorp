#pragma once

#include "camera.h"
#include "sparse.h"
#include "collision.h"
#include "game/entity.h"

typedef For_Result (*For_Each_Entity_Callback)(Entity *e, void *user_data);

struct Game_World {
    static constexpr u32 MAX_NAME_SIZE = 64;

    static constexpr u32 MAX_STATIC_MESHES     = 1024;
    static constexpr u32 MAX_DIRECT_LIGHTS     = 4;
    static constexpr u32 MAX_POINT_LIGHTS      = 32;
    static constexpr u32 MAX_SOUND_EMITTERS_2D = 32;
    static constexpr u32 MAX_SOUND_EMITTERS_3D = 128;
    static constexpr u32 MAX_PORTALS           = 128;
    static constexpr u32 MAX_AABBS = 2048;
    
    char name[MAX_NAME_SIZE];

	f32 dt;

	Player player;
	Camera camera;
	Camera ed_camera;

	Skybox skybox;

	Sparse<Static_Mesh>      static_meshes;
	Sparse<Point_Light>      point_lights;
	Sparse<Direct_Light>     direct_lights;
	Sparse<Sound_Emitter_2D> sound_emitters_2d;
	Sparse<Sound_Emitter_3D> sound_emitters_3d;
	Sparse<Portal>           portals;
    
	Sparse<AABB> aabbs;
};

inline Game_World World;

void create_world(Game_World &world);
void save_level(Game_World &world);
void load_level(Game_World &world, const char *path);
void tick(Game_World &world, f32 dt);
Camera &active_camera(Game_World &world);

Entity *create_entity(Game_World &world, Entity_Type e_type);
Entity *find_entity_by_eid(Game_World &world, eid eid);

void draw_world(const Game_World &world);
void draw_entity(const Entity &e);

void for_each_entity(Game_World &world, For_Each_Entity_Callback callback, void *user_data = null);
