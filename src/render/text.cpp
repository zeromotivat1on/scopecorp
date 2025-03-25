#include "pch.h"
#include "text.h"
#include "font.h"

#include "os/file.h"

#include "math/vector.h"
#include "math/matrix.h"

#include "render/draw.h"
#include "render/viewport.h"
#include "render/render_registry.h"

Text_Draw_Command *create_default_text_draw_command(Font_Atlas *atlas) {
	// @Cleanup: create separate container for draw commands.
	auto *command = push_struct(pers, Text_Draw_Command);
	*command = Text_Draw_Command();

    command->flags = DRAW_FLAG_CULL_FACE_TEST | DRAW_FLAG_BLEND_TEST | DRAW_FLAG_DEPTH_TEST | DRAW_FLAG_ENTIRE_BUFFER | DRAW_FLAG_RESET;
	command->draw_mode    = DRAW_TRIANGLE_STRIP;
    command->polygon_mode = POLYGON_FILL;
    command->cull_face_test.type    = CULL_FACE_BACK;
    command->cull_face_test.winding = WINDING_COUNTER_CLOCKWISE;
    command->blend_test.source      = BLEND_TEST_SOURCE_ALPHA;
    command->blend_test.destination = BLEND_TEST_ONE_MINUS_SOURCE_ALPHA;
    command->depth_test.function = DEPTH_TEST_LESS;
    command->depth_test.mask     = DEPTH_TEST_DISABLE;

    const s32 material_index = material_index_list.text;
    set_material_uniform_value(material_index, "u_charmap",    command->charmap);
	set_material_uniform_value(material_index, "u_transforms", command->transforms);
	set_material_uniform_value(material_index, "u_projection", &command->projection);

    const vec3 default_color = vec3_white;
    set_material_uniform_value(material_index_list.text, "u_color", &default_color);

    const auto &material  = render_registry.materials[material_index];
    command->shader_index  = material.shader_index;
    command->texture_index = atlas->texture_index;
    command->uniform_count = material.uniform_count;

    for (s32 i = 0; i < material.uniform_count; ++i) {
        command->uniform_indices[i]       = material.uniform_indices[i];
        command->uniform_value_offsets[i] = material.uniform_value_offsets[i];
    }
    
	command->atlas = atlas;
    
	f32 vertices[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};
	Vertex_Component_Type components[] = { VERTEX_F32_2 };
	command->vertex_buffer_index = create_vertex_buffer(components, c_array_count(components), vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

	return command;
}

void draw_text_immediate(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color) {
	const auto *atlas = command->atlas;

	//render_registry.materials[material_index_list.text].texture_index = atlas->texture_index;
	set_material_uniform_value(material_index_list.text, "u_color", &color);

	s32 work_index = 0;
	f32 x = pos.x;
	f32 y = pos.y;

	for (u32 i = 0; i < text_size; ++i) {
		const char c = text[i];

		if (c == ' ') {
			x += atlas->space_advance_width;
			continue;
		}

		if (c == '\t') {
			x += 4 * atlas->space_advance_width;
			continue;
		}

		if (c == '\n') {
			x = pos.x;
			y -= atlas->line_height;
			continue;
		}

		assert((u32)c >= atlas->start_charcode);
		assert((u32)c <= atlas->end_charcode);

		const u32 ci = c - atlas->start_charcode; // correctly shifted index
		const Font_Glyph_Metric *metric = atlas->metrics + ci;

		const f32 gw = (f32)atlas->font_size;
		const f32 gh = (f32)atlas->font_size;
		const f32 gx = x + metric->offset_x;
		const f32 gy = y - (gh + metric->offset_y);

		mat4 *transform = command->transforms + work_index;
		transform->identity().translate(vec3(gx, gy, 0.0f)).scale(vec3(gw, gh, 0.0f));

		command->charmap[work_index] = ci;

		if (++work_index >= TEXT_DRAW_BATCH_SIZE) {
            set_material_uniform_value(material_index_list.text, "u_charmap",    command->charmap);
            set_material_uniform_value(material_index_list.text, "u_transforms", command->transforms);
            
			command->instance_count = work_index;
			draw(command);
			work_index = 0;
		}

		x += metric->advance_width;
	}

	if (work_index > 0) {
        set_material_uniform_value(material_index_list.text, "u_charmap",    command->charmap);
        set_material_uniform_value(material_index_list.text, "u_transforms", command->transforms);
        
		command->instance_count = work_index;
		draw(command);
	}
}

void draw_text_immediate_with_shadow(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color) {
	draw_text_immediate(command, text, text_size, pos + shadow_offset, shadow_color);
	draw_text_immediate(command, text, text_size, pos, color);
}

void on_viewport_resize(Text_Draw_Command *command, Viewport *viewport) {
	command->projection = mat4_orthographic(0, viewport->width, 0, viewport->height, -1, 1);
    set_material_uniform_value(material_index_list.text, "u_projection", &command->projection);
}
