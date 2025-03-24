#include "pch.h"
#include "render/draw.h"
#include "render/debug_draw.h"
#include "render/viewport.h"
#include "render/render_registry.h"

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

void draw_entity(const Entity *e) {
    Draw_Command command{};
    command.flags = DRAW_FLAG_SCISSOR_TEST | DRAW_FLAG_CULL_FACE_TEST | DRAW_FLAG_BLEND_TEST | DRAW_FLAG_DEPTH_TEST | DRAW_FLAG_STENCIL_TEST | DRAW_FLAG_ENTIRE_BUFFER | DRAW_FLAG_RESET;
    command.draw_mode    = DRAW_TRIANGLES;
    command.polygon_mode = POLYGON_FILL;
    command.scissor_test.x      = viewport.x;
    command.scissor_test.y      = viewport.y;
    command.scissor_test.width  = viewport.width;
    command.scissor_test.height = viewport.height;
    command.cull_face_test.type    = CULL_FACE_BACK;
    command.cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command.blend_test.source      = BLEND_TEST_SOURCE_ALPHA;
    command.blend_test.destination = BLEND_TEST_ONE_MINUS_SOURCE_ALPHA;
    command.depth_test.function = DEPTH_TEST_LESS;
    command.depth_test.mask     = DEPTH_TEST_ENABLE;
    command.stencil_test.operation.stencil_failed = STENCIL_TEST_KEEP;
    command.stencil_test.operation.depth_failed   = STENCIL_TEST_KEEP;
    command.stencil_test.operation.both_passed    = STENCIL_TEST_REPLACE;
    command.stencil_test.function.type       = STENCIL_TEST_ALWAYS;
    command.stencil_test.function.comparator = 1;
    command.stencil_test.function.mask       = 0xFF;
    command.stencil_test.mask = 0x00;
    command.frame_buffer_index  = viewport.frame_buffer_index;
    command.vertex_buffer_index = e->draw_data.vbi;
    command.index_buffer_index  = e->draw_data.ibi;

    // @Todo: create function to fill draw command from material.
    const auto &material  = render_registry.materials[e->draw_data.mti];
    command.shader_index  = material.shader_index;
    command.texture_index = material.texture_index;
    command.uniform_count = material.uniform_count;

    for (s32 i = 0; i < material.uniform_count; ++i) {
        command.uniform_indices[i]       = material.uniform_indices[i];
        command.uniform_value_offsets[i] = material.uniform_value_offsets[i];
    }

    if (e->flags & ENTITY_FLAG_WIREFRAME) {
        command.flags &= ~DRAW_FLAG_CULL_FACE_TEST;
        command.polygon_mode = POLYGON_LINE;
    }
    
    if (e->flags & ENTITY_FLAG_OUTLINE) {
        command.stencil_test.mask = 0xFF;
    }
    
	enqueue_draw_command(&world_draw_queue, &command);

    if (e->flags & ENTITY_FLAG_OUTLINE) {        
        command.flags &= ~DRAW_FLAG_DEPTH_TEST;
        command.stencil_test.function.type       = STENCIL_TEST_NOT_EQUAL;
        command.stencil_test.function.comparator = 1;
        command.stencil_test.function.mask       = 0xFF;
        command.stencil_test.mask                = 0x00;

        const auto &material  = render_registry.materials[material_index_list.outline];
        command.shader_index  = material.shader_index;
        command.texture_index = material.texture_index;
        command.uniform_count = material.uniform_count;

        const Camera *camera = desired_camera(world);
        const mat4 view = camera_view(camera);
        const mat4 proj = camera_projection(camera);
        const mat4 mvp = mat4_transform(e->location, e->rotation, e->scale * 1.1f) * view * proj;
        set_material_uniform_value(material_index_list.outline, "u_transform", mvp.ptr());

        const vec3 color = vec3_yellow;
        set_material_uniform_value(material_index_list.outline, "u_color", &color);

        for (s32 i = 0; i < material.uniform_count; ++i) {
            command.uniform_indices[i]       = material.uniform_indices[i];
            command.uniform_value_offsets[i] = material.uniform_value_offsets[i];
        }
        
        enqueue_draw_command(&world_draw_queue, &command);
    }
}
