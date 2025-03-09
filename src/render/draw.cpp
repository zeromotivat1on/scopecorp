#include "pch.h"
#include "render/draw.h"
#include "render/debug_draw.h"

#include "game/world.h"
#include "game/entities.h"

#include "log.h"
#include "profile.h"
#include "memory_storage.h"

void init_draw_queue(Draw_Queue *queue, s32 size) {
	queue->commands = push_array(pers, size, Draw_Command);
	queue->count = 0;
}

void enqueue_draw_command(Draw_Queue *queue, const Draw_Command *command) {
	assert(queue->count < MAX_DRAW_QUEUE_SIZE);
	const s32 index = queue->count++;
	memcpy(queue->commands + index, command, sizeof(Draw_Command));
}

void flush(Draw_Queue *queue) {
    PROFILE_SCOPE("Flush Draw Queue");
    
	for (s32 i = 0; i < queue->count; ++i) draw(queue->commands + i);
	queue->count = 0;
}

void draw_world(const World *world) {
    PROFILE_SCOPE(__FUNCTION__);
    
	draw_entity(&world->skybox);

	for (s32 i = 0; i < world->static_meshes.count; ++i)
		draw_entity(world->static_meshes.items + i);

	draw_entity(&world->player);
}

static Draw_Command draw_command_for(const Entity *e) {
    Draw_Command command;
    command.flags = e->draw_flags;
    
	switch (e->type) {
	case E_STATIC_MESH: {
		const auto *mesh = (Static_Mesh *)e;
		command.vertex_buffer_index = mesh->vertex_buffer_index;
		command.index_buffer_index  = mesh->index_buffer_index;
		command.material_index      = mesh->material_index;
		return command;
	}
	case E_PLAYER: {
		const auto *player = (Player *)e;
		command.vertex_buffer_index = player->vertex_buffer_index;
		command.index_buffer_index  = player->index_buffer_index;
		command.material_index      = player->material_index;
		return command;
	}
	case E_SKYBOX: {
		const auto *skybox = (Skybox *)e;
		command.vertex_buffer_index = skybox->vertex_buffer_index;
		command.index_buffer_index  = skybox->index_buffer_index;
		command.material_index      = skybox->material_index;
		return command;
	}
	default:
		error("Failed to create draw command for entity of unknown type %d", e->type);
		return {};
	}
}

void draw_entity(const Entity *e) {
	const auto command = draw_command_for(e);
	enqueue_draw_command(&world_draw_queue, &command);
}
