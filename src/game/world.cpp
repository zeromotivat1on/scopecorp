#include "pch.h"
#include "game/world.h"
#include "game/game.h"
#include "game/player_control.h"

#include "log.h"
#include "profile.h"

#include "render/draw.h"
#include "render/material.h"

void init_world(World *world) {
	*world = World();

	world->static_meshes = Sparse_Array<Static_Mesh>(MAX_STATIC_MESHES);
	world->aabbs         = Sparse_Array<AABB>(MAX_AABBS);
}

static void try_mark_mouse_picked_entity(Entity *e) {
    if (e->id == world->selected_entity_id) {
        e->flags |= ENTITY_FLAG_OUTLINE;
    } else {
        e->flags &= ~ENTITY_FLAG_OUTLINE;
    }
}

void tick(World *world, f32 dt) {
    PROFILE_SCOPE("Tick World");
    
	world->dt = dt;

	Camera *camera = desired_camera(world);
	const mat4 view = camera_view(camera);
	const mat4 proj = camera_projection(camera);

	auto &skybox = world->skybox;
    try_mark_mouse_picked_entity(&skybox);
	skybox.uv_offset = camera->eye;
	set_material_uniform_value(skybox.draw_data.mti, "u_scale", &skybox.uv_scale);
	set_material_uniform_value(skybox.draw_data.mti, "u_offset", &skybox.uv_offset);

	for (s32 i = 0; i < world->static_meshes.count; ++i) {
		auto *mesh = world->static_meshes.items + i;
        try_mark_mouse_picked_entity(mesh);
		mesh->mvp = mat4_transform(mesh->location, mesh->rotation, mesh->scale) * view * proj;
		set_material_uniform_value(mesh->draw_data.mti, "u_transform", mesh->mvp.ptr());
	}

	tick_player(world);

    auto &player = world->player;
    try_mark_mouse_picked_entity(&player);
	player.mvp = mat4_transform(player.location, player.rotation, player.scale) * view * proj;
	set_material_uniform_value(player.draw_data.mti, "u_transform", player.mvp.ptr());

	if (game_state.mode == MODE_GAME) {
		// Editor camera should follow game one during gameplay.
		world->ed_camera = world->camera;
	}
}

Camera *desired_camera(World *world) {
	if (game_state.mode == MODE_GAME)   return &world->camera;
	if (game_state.mode == MODE_EDITOR) return &world->ed_camera;

	error("Failed to get desired camera from world in unknown game mode %d", game_state.mode);
	return null;
}

s32 create_static_mesh(World *world) {
    const s32 index = world->static_meshes.add_default();
    world->static_meshes[index].aabb_index = world->aabbs.add_default();
    return index;
}
