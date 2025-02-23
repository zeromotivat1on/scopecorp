#pragma once

#include "camera.h"
#include "entities.h"

inline constexpr s32 MAX_ENTITIES = 1024;

struct World
{
    f32 dt;
    
    Player player;
    Camera camera;
    Camera ed_camera;

    //Entity* entities;
    //s32 entity_count;
};

World* create_world();
void tick(World* world, f32 dt);
Camera* desired_camera(World* world);

/*
Entity* new_entity(World* world);
Entity* find_entity(World* world, s32 id);
void delete_entity(World* world, s32 id);
*/
