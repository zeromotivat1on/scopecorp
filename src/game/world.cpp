#include "pch.h"
#include "world.h"
#include "game.h"
#include "player_control.h"
#include "memory_storage.h"

World* create_world() {
    World* world = alloc_struct_persistent(World);
    *world = World();
    return world;
}

void tick(World* world, f32 dt) {
    world->dt = dt;
    tick(&world->player, dt);

    if (game_state.mode == MODE_GAME) {
        // Editor camera should follow game one during gameplay.
        world->ed_camera = world->camera;
    }
}

Camera* desired_camera(World* world) {
    if (game_state.mode == MODE_GAME) return &world->camera;
    if (game_state.mode == MODE_EDITOR) return &world->ed_camera;
    assert(false); // never gonna happen
    return null;
}
