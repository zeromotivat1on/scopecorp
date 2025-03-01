#pragma once

#include "camera.h"
#include "slot_array.h"
#include "game/entities.h"

inline constexpr s32 MAX_STATIC_MESHES = 1024;

struct World {
    f32 dt;
    
    Player player;
    Camera camera;
    Camera ed_camera;

    Slot_Array<Static_Mesh> static_meshes;
};

inline World* world = null;

World* create_world();
Camera* desired_camera(World* world);
void tick(World* world, f32 dt);
