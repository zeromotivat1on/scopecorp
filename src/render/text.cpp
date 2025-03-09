#include "pch.h"
#include "text.h"
#include "font.h"

#include "os/file.h"

#include "math/vector.h"
#include "math/matrix.h"

#include "render/draw.h"
#include "render/render_registry.h"
#include "render/viewport.h"

Text_Draw_Command *create_default_text_draw_command(Font_Atlas *atlas) {
	// @Cleanup: create separate container for draw commands.
	auto *command = push_struct(pers, Text_Draw_Command);
	*command = Text_Draw_Command();

	command->flags = DRAW_FLAG_ENTIRE_BUFFER | DRAW_FLAG_IGNORE_DEPTH;
	command->draw_mode = DRAW_TRIANGLE_STRIP;
	command->material_index = material_index_list.text;
	command->atlas = atlas;
	set_material_uniform_value(command->material_index, "u_charmap", command->charmap);
	set_material_uniform_value(command->material_index, "u_transforms", command->transforms);
	set_material_uniform_value(command->material_index, "u_projection", &command->projection);

	f32 vertices[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};
	Vertex_Attrib_Type attribs[] = { VERTEX_ATTRIB_F32_V2 };
	command->vertex_buffer_index = create_vertex_buffer(attribs, c_array_count(attribs), vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);

	return command;
}

void draw_text_immediate(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color) {
	const auto *atlas = command->atlas;

	// @Cleanup: create function for this.
	render_registry.materials[command->material_index].texture_index = atlas->texture_index;
	set_material_uniform_value(command->material_index, "u_text_color", &color);

	// As charmap and transforms array are static arrays and won't be moved,
	// we can just make uniforms dirty, so they will be synced with gpu later.
	mark_material_uniform_dirty(command->material_index, "u_charmap");
	mark_material_uniform_dirty(command->material_index, "u_transforms");

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
			command->instance_count = work_index;
			draw(command);
			work_index = 0;
		}

		x += metric->advance_width;
	}

	if (work_index > 0) {
		command->instance_count = work_index;
		draw(command);
	}
}

void draw_text_immediate_with_shadow(Text_Draw_Command *command, const char *text, u32 text_size, vec2 pos, vec3 color, vec2 shadow_offset, vec3 shadow_color)
{
	draw_text_immediate(command, text, text_size, pos + shadow_offset, shadow_color);
	draw_text_immediate(command, text, text_size, pos, color);
}

void on_viewport_resize(Text_Draw_Command *command, Viewport *viewport) {
	command->projection = mat4_orthographic(0, viewport->width, 0, viewport->height, -1, 1);
	mark_material_uniform_dirty(command->material_index, "u_projection");
}
