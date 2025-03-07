#pragma once

inline constexpr s32 MAX_DRAW_QUEUE_SIZE = 64;

struct Entity;
struct World;

enum Draw_Mode {
	DRAW_TRIANGLES,
	DRAW_TRIANGLE_STRIP,
	DRAW_LINES,
};

enum Draw_Command_Flags : u32 {
	DRAW_FLAG_IGNORE_DEPTH = 0x1,
    DRAW_FLAG_WIREFRAME    = 0x2, // draw as lines and disable face culling
};

struct Draw_Command {
	u32 flags = 0;
	Draw_Mode draw_mode = DRAW_TRIANGLES;

	s32 vertex_buffer_index = INVALID_INDEX;
	s32 index_buffer_index  = INVALID_INDEX;
	s32 material_index      = INVALID_INDEX;

	s32 instance_count = 1;
};

struct Draw_Queue {
	Draw_Command *commands = null;
	s32 count = 0;
};

inline Draw_Queue draw_queue;

void init_draw_queue(Draw_Queue *queue);
void enqueue_draw_command(Draw_Queue *queue, const Draw_Command *command);
void enqueue_draw_world(Draw_Queue *queue, const World *world);
void enqueue_draw_entity(Draw_Queue *queue, const Entity *e);
void flush_draw_commands(Draw_Queue *queue);

void draw(const Draw_Command *command);
