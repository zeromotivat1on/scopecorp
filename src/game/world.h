#pragma once

#include "camera.h"
#include "sparse_array.h"
#include "game/entities.h"

inline constexpr s32 MAX_STATIC_MESHES = 1024;

struct World {
    f32 dt;
    
    Player player;
    Camera camera;
    Camera ed_camera;

    Skybox skybox;
    
    Sparse_Array<Static_Mesh> static_meshes;
};

inline World* world = null;

void init_world(World* world);
void tick(World* world, f32 dt);
Camera* desired_camera(World* world);
