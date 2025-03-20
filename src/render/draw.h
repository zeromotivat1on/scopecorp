 #pragma once

inline constexpr s32 MAX_DRAW_QUEUE_SIZE = 1024;

struct Entity;
struct World;

enum Draw_Mode {
	DRAW_TRIANGLES,
	DRAW_TRIANGLE_STRIP,
	DRAW_LINES,
};

enum Draw_Command_Flags : u32 {
	DRAW_FLAG_IGNORE_DEPTH  = 0x1, // do not write to depth buffer
    DRAW_FLAG_WIREFRAME     = 0x2, // draw as lines and disable face culling
    DRAW_FLAG_ENTIRE_BUFFER = 0x4, // ignore offset/count, draw entire vertex/index buffer
    DRAW_FLAG_SKIP_DRAW     = 0x8,
};

struct Draw_Command {
	u32 flags = 0;
	Draw_Mode draw_mode = DRAW_TRIANGLES;

	s32 vertex_buffer_index = INVALID_INDEX;
	s32 index_buffer_index  = INVALID_INDEX;
	s32 material_index      = INVALID_INDEX;

    s32 draw_count  = 0; // amount of vertices/indices to draw
    s32 draw_offset = 0; // offset in vertex/index buffer
    
	s32 instance_count = 1;
};

struct Draw_Queue {
	Draw_Command *commands = null;
	s32 count = 0;
};

inline Draw_Queue world_draw_queue;

void init_draw_queue(Draw_Queue *queue, s32 size);
void enqueue_draw_command(Draw_Queue *queue, const Draw_Command *command);
void flush(Draw_Queue *queue);

void draw_world(const World *world);
void draw_entity(const Entity *e);

void draw(const Draw_Command *command);
