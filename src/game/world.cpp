#include "pch.h"
#include "world.h"
#include "game.h"
#include "player_control.h"

World* create_world()
{
    World* world = alloc_struct_persistent(World);
    *world = World();
    return world;
}

void tick(World* world, f32 dt)
{
    world->dt = dt;
    tick(&world->player, dt);

    if (game_state.mode == MODE_GAME)
    {
        // Editor camera should follow game one during gameplay.
        world->ed_camera = world->camera;
    }
}

Camera* desired_camera(World* world)
{
    if (game_state.mode == MODE_GAME) return &world->camera;
    if (game_state.mode == MODE_EDITOR) return &world->ed_camera;
    assert(false); // never gonna happen
    return null;
}

/*
Entity* new_entity(World* world)
{
    static s32 id = 0;
    assert(world->entity_count < MAX_ENTITIES);
    
    Entity* e = world->entities + world->entity_count++;
    e->id = id++;
    return e;
}

Entity* find_entity(World* world, s32 id)
{
    assert(id != INVALID_ENTITY_ID);

    // @Cleanup: linear search is bad for possibly large entity count.
    for (s32 i = 0; i < world->entity_count; ++i)
    {
        if (world->entities[i].id == id)
            return world->entities + i;
    }

    return null;
}

void delete_entity(World* world, s32 id)
{
    Entity* entity_to_delete = find_entity(world, id);
    if (!entity_to_delete) return;
    
    Entity* last_entity = world->entities + --world->entity_count;
    memcpy(entity_to_delete, last_entity, sizeof(Entity));
    
    // We've moved last entity to new delete position, invalidate its id.
    last_entity->id = INVALID_ENTITY_ID;
}
*/
