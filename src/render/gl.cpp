#include "pch.h"
#include "render/gfx.h"
#include "render/glad.h"
#include "render/draw.h"
#include "render/text.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"
#include "render/index_buffer.h"
#include "render/vertex_buffer.h"
#include "render/render_registry.h"
#include "render/viewport.h"

#include "log.h"
#include "font.h"
#include "profile.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_sprintf.h"

#include "os/file.h"

#include <string.h>

void set_gfx_features(u32 flags) {
    gfx_features = flags;
    
	if (flags & GFX_FLAG_BLEND) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (flags & GFX_FLAG_CULL_BACK_FACE) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	if (flags & GFX_FLAG_WINDING_CCW) {
		glFrontFace(GL_CCW);
	}

	if (flags & GFX_FLAG_SCISSOR) {
		glEnable(GL_SCISSOR_TEST);
	}

	if (flags & GFX_FLAG_DEPTH) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}
}

void add_gfx_features(u32 flags) {
    gfx_features |= flags;
    set_gfx_features(gfx_features);
}

void remove_gfx_features(u32 flags) {
    gfx_features &= ~flags;
    set_gfx_features(gfx_features);
}

void clear_screen(vec4 color) {
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static s32 gl_draw_mode(Draw_Mode mode) {
	switch (mode) {
	case DRAW_TRIANGLES:      return GL_TRIANGLES;
	case DRAW_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
	case DRAW_LINES:          return GL_LINES;
	default:
		error("Failed to get open gl draw mode from given draw mode %d", mode);
		return -1;
	}
}

void draw(const Draw_Command *command) {
	if (command->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_FALSE);

    if (command->flags & DRAW_FLAG_WIREFRAME) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
	const s32 draw_mode = gl_draw_mode(command->draw_mode);

	const auto &material = render_registry.materials[command->material_index];
	const auto &shader   = render_registry.shaders[material.shader_index];
	glUseProgram(shader.id);

	for (s32 i = 0; i < material.uniform_count; ++i)
		sync_uniform(material.uniforms + i);

	if (material.texture_index != INVALID_INDEX) {
		const auto &texture = render_registry.textures[material.texture_index];
		if (texture.flags & TEXTURE_FLAG_2D_ARRAY) glBindTexture(GL_TEXTURE_2D_ARRAY, texture.id);
		else glBindTexture(GL_TEXTURE_2D, texture.id);
		glActiveTexture(GL_TEXTURE0);
	}

	const auto &vertex_buffer = render_registry.vertex_buffers[command->vertex_buffer_index];
	glBindVertexArray(vertex_buffer.id);

	if (command->index_buffer_index != INVALID_INDEX) {
		const auto &index_buffer = render_registry.index_buffers[command->index_buffer_index];
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.id);

        s32 index_count = 0;
        const void* index_ptr = null;
        if (command->flags & DRAW_FLAG_ENTIRE_BUFFER) {
            index_count = index_buffer.index_count;
        } else {
            index_count = command->draw_count;
            index_ptr = 0; // @Todo: find offset pointer to index data using index offset
        }
        
        glDrawElementsInstanced(draw_mode, index_count, GL_UNSIGNED_INT, index_ptr, command->instance_count);
	} else {
        s32 vertex_count = 0;
        s32 vertex_first = 0;
        if (command->flags & DRAW_FLAG_ENTIRE_BUFFER) {
            vertex_count = vertex_buffer.vertex_count;
        } else {
            vertex_count = command->draw_count;
            vertex_first = command->draw_offset;
        }
        
		glDrawArraysInstanced(draw_mode, vertex_first, vertex_count, command->instance_count);
	}

    {
        PROFILE_SCOPE("Reset GL state after draw call");
        
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }
}

void resize_viewport(Viewport *viewport, s16 width, s16 height)
{
	switch (viewport->aspect_type) {
    case VIEWPORT_FILL_WINDOW:
        viewport->width = width;
		viewport->height = height;
        break;
	case VIEWPORT_4X3:
		viewport->width = width;
		viewport->height = height;

		if (width * 3 > height * 4) {
			viewport->width = height * 4 / 3;
			viewport->x = (width - viewport->width) / 2;
		} else {
			viewport->height = width * 3 / 4;
			viewport->y = (height - viewport->height) / 2;
		}

		break;
	default:
		error("Failed to resize viewport with unknown aspect type %d", viewport->aspect_type);
		break;
	}

	glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
	glScissor(viewport->x, viewport->y, viewport->width, viewport->height);
}

static s32 gl_usage(Buffer_Usage_Type type) {
	switch (type) {
	case BUFFER_USAGE_STATIC:  return GL_STATIC_DRAW;
	case BUFFER_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW;
	case BUFFER_USAGE_STREAM:  return GL_STREAM_DRAW;
	default:
		error("Failed to get open gl usage from given buffer usage %d", type);
		return -1;
	}
}

s32 create_vertex_buffer(Vertex_Attrib_Type *attribs, s32 attrib_count, const void *data, s32 vertex_count, Buffer_Usage_Type usage_type) {
	assert(attrib_count <= MAX_VERTEX_LAYOUT_ATTRIBS);

	Vertex_Buffer buffer = {0};
	buffer.usage_type = usage_type;
	buffer.vertex_count = vertex_count;
	memcpy(&buffer.layout.attribs, attribs, attrib_count * sizeof(Vertex_Attrib_Type));

	glGenBuffers(1, &buffer.handle);
	glGenVertexArrays(1, &buffer.id);

	glBindVertexArray(buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);

	s32 vertex_size = 0;
	for (s32 i = 0; i < attrib_count; ++i)
		vertex_size += vertex_attrib_size(attribs[i]);

    const s32 usage = gl_usage(usage_type);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * vertex_size, data, usage);

	for (s32 i = 0; i < attrib_count; ++i) {
		const s32 dimension = vertex_attrib_dimension(attribs[i]);

		s32 offset = 0;
		for (s32 j = 0; j < i; ++j)
			offset += vertex_attrib_size(attribs[j]);

		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, dimension, GL_FLOAT, GL_FALSE, vertex_size, (void *)(u64)offset);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return render_registry.vertex_buffers.add(buffer);
}

void set_vertex_buffer_data(s32 vbi, const void *data, u32 size, u32 offset) {
    const auto &buffer = render_registry.vertex_buffers[vbi];

    glBindVertexArray(buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);

    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

s32 create_index_buffer(u32 *indices, s32 count, Buffer_Usage_Type usage_type) {
	Index_Buffer buffer;
	buffer.index_count = count;
	buffer.usage_type = usage_type;

	glGenBuffers(1, &buffer.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.id);

	const s32 usage = gl_usage(usage_type);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u32), indices, usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return render_registry.index_buffers.add(buffer);
}

static u32 gl_create_shader(GLenum type, const char *src) {
	u32 shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, null);
	glCompileShader(shader);

	s32 success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		const char *shader_name;
		if (type == GL_VERTEX_SHADER) shader_name = "vertex";
		else if (type == GL_FRAGMENT_SHADER) shader_name = "fragment";

		char info_log[512];
		glGetShaderInfoLog(shader, sizeof(info_log), null, info_log);
		error("Failed to compile shader %s, gl reason %s", shader_name, info_log);
		return INVALID_INDEX;
	}

	return shader;
}

static u32 gl_link_program(u32 vertex_shader, u32 fragment_shader) {
	u32 program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	s32 success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info_log[512];
		glGetProgramInfoLog(program, sizeof(info_log), null, info_log);
		error("Failed to link shader program, gl reason %s", info_log);
		return INVALID_INDEX;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

s32 create_shader(const char *path) {
	char timer_string[256];
	stbsp_snprintf(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
	SCOPE_TIMER(timer_string);

	Shader shader;
	shader.path = path;

	u64 shader_size = 0;
	u8 *shader_src = (u8 *)push(temp, MAX_SHADER_SIZE);
	defer { pop(temp, MAX_SHADER_SIZE); };

	if (!read_file(path, shader_src, MAX_SHADER_SIZE, &shader_size))
		return INVALID_INDEX;

	shader_src[shader_size] = '\0';

	char *vertex_src   = (char *)push(temp, MAX_SHADER_SIZE);
	char *fragment_src = (char *)push(temp, MAX_SHADER_SIZE);
    defer {
        pop(temp, MAX_SHADER_SIZE);
        pop(temp, MAX_SHADER_SIZE);
    };

	if (!parse_shader_source((char *)shader_src, vertex_src, fragment_src)) {
        error("Failed to parse shader %s", path);
        return INVALID_INDEX;
    }

	const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
	const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
	shader.id = gl_link_program(vertex_shader, fragment_shader);

	return render_registry.shaders.add(shader);
}

bool recreate_shader(s32 shader_index) {
    auto &shader = render_registry.shaders[shader_index];
    
	u64 shader_size = 0;
	u8 *shader_src = (u8 *)push(temp, MAX_SHADER_SIZE);
	defer { pop(temp, MAX_SHADER_SIZE); };

	if (!read_file(shader.path, shader_src, MAX_SHADER_SIZE, &shader_size))
		return false;

	shader_src[shader_size] = '\0';

    char *vertex_src   = (char *)push(temp, MAX_SHADER_SIZE);
	char *fragment_src = (char *)push(temp, MAX_SHADER_SIZE);
    defer {
        pop(temp, MAX_SHADER_SIZE);
        pop(temp, MAX_SHADER_SIZE);
    };


	if (!parse_shader_source((char *)shader_src, vertex_src, fragment_src)) {
        error("Failed to parse shader %s", shader.path);
        return false;
    }
    
	// We are free to delete old shader program at this stage.
	glDeleteProgram(shader.id);

	const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
	const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
	shader.id = gl_link_program(vertex_shader, fragment_shader);

	return true;
}

s32 find_shader_by_file(Shader_Index_List *list, const char *path) {
	const s32 item_size = sizeof(s32);
	const s32 list_size = sizeof(Shader_Index_List);

	u8 *data = (u8 *)list;
	s32 pos = 0;

	while (pos < list_size) {
		s32 shader_index = *(s32 *)(data + pos);
		assert(shader_index < MAX_SHADERS);

		const auto &shader = render_registry.shaders[shader_index];
		if (strcmp(shader.path, path) == 0) return shader_index;

		pos += item_size;
	}

	return INVALID_INDEX;
}

void add_material_uniforms(s32 material_index, const Uniform *uniforms, s32 count) {
	auto &material     = render_registry.materials[material_index];
	const auto &shader = render_registry.shaders[material.shader_index];
	assert(material.uniform_count + count <= MAX_MATERIAL_UNIFORMS);

	for (s32 i = 0; i < count; ++i) {
		auto *src_uniform = uniforms + i;
		auto *dst_uniform = material.uniforms + material.uniform_count + i;
		memcpy(dst_uniform, src_uniform, sizeof(Uniform));

        dst_uniform->location = glGetUniformLocation(shader.id, dst_uniform->name);
        if (dst_uniform->location == -1)
            error("Failed to get uniform location with invalid name %s", dst_uniform->name);
	}

	material.uniform_count += count;
}

Uniform *find_material_uniform(s32 material_index, const char *name) {
	auto &material = render_registry.materials[material_index];

	Uniform *uniform = null;
	for (s32 i = 0; i < material.uniform_count; ++i) {
		if (strcmp(material.uniforms[i].name, name) == 0)
			return material.uniforms + i;
	}

	return null;
}

void set_material_uniform_value(s32 material_index, const char *name, const void *data) {
	Uniform *uniform = find_material_uniform(material_index, name);
	if (!uniform) {
		error("Failed to set material uniform %s value as its not found", name);
		return;
	}

	uniform->value = data;
	uniform->flags |= UNIFORM_FLAG_DIRTY;
}

void mark_material_uniform_dirty(s32 material_index, const char *name) {
	Uniform *uniform = find_material_uniform(material_index, name);
	if (!uniform) {
		error("Failed to mark material uniform %s dirty as its not found", name);
		return;
	}

	uniform->flags |= UNIFORM_FLAG_DIRTY;
}

void sync_uniform(const Uniform *uniform) {
	if (uniform->type == UNIFORM_NULL) {
		warn("Tried to sync uniform of null type %p", uniform);
		return;
	}

	if (!(uniform->flags & UNIFORM_FLAG_DIRTY)) return;

	switch (uniform->type) {
	case UNIFORM_U32:
		glUniform1uiv(uniform->location, uniform->count, (u32 *)uniform->value);
		break;
    case UNIFORM_F32:
		glUniform1fv(uniform->location, uniform->count, (f32 *)uniform->value);
		break;
	case UNIFORM_F32_VEC2:
		glUniform2fv(uniform->location, uniform->count, (f32 *)uniform->value);
		break;
	case UNIFORM_F32_VEC3:
		glUniform3fv(uniform->location, uniform->count, (f32 *)uniform->value);
		break;
	case UNIFORM_F32_MAT4:
		glUniformMatrix4fv(uniform->location, uniform->count, GL_FALSE, (f32 *)uniform->value);
		break;
	default:
		error("Failed to sync uniform of unknown type %d", uniform->type);
		break;
	}
}

static u32 gl_create_texture(void *data, s32 width, s32 height) {
	u32 texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

s32 create_texture(const char *path) {
	char timer_string[256];
	stbsp_snprintf(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
	SCOPE_TIMER(timer_string);

	Texture texture = {0};
	texture.path = path;

	stbi_set_flip_vertically_on_load(true);
	defer { stbi_set_flip_vertically_on_load(false); };

	u64 buffer_size = 0;
	u8 *buffer = (u8 *)push(temp, MAX_TEXTURE_SIZE);
	defer { pop(temp, MAX_TEXTURE_SIZE); };

	if (!read_file(path, buffer, MAX_TEXTURE_SIZE, &buffer_size)) {
		return INVALID_INDEX;
	}

	u8 *data = stbi_load_from_memory(buffer, (s32)buffer_size, &texture.width, &texture.height, &texture.color_channel_count, 4);
	if (!data) {
		error("Failed to load texture %s, stbi reason %s", path, stbi_failure_reason());
		return INVALID_INDEX;
	}

	texture.id = gl_create_texture(data, texture.width, texture.height);

	return render_registry.textures.add(texture);
}

Font_Atlas *bake_font_atlas(const Font *font, u32 start_charcode, u32 end_charcode, s16 font_size) {
	Font_Atlas *atlas = push_struct(pers, Font_Atlas);
	atlas->font = font;
	atlas->texture_index = INVALID_INDEX;

	const u32 charcode_count = end_charcode - start_charcode + 1;
	atlas->metrics = push_array(pers, charcode_count, Font_Glyph_Metric);
	atlas->start_charcode = start_charcode;
	atlas->end_charcode = end_charcode;

	Texture texture;
	glGenTextures(1, &texture.id);
	texture.flags |= TEXTURE_FLAG_2D_ARRAY;
	texture.width = font_size;
	texture.height = font_size;
	texture.color_channel_count = 4;
	texture.path = "texture for glyph batch rendering";

	atlas->texture_index = render_registry.textures.add(texture);

	rescale_font_atlas(atlas, font_size);

	return atlas;
}

void rescale_font_atlas(Font_Atlas *atlas, s16 font_size) {
	if (atlas->texture_index == INVALID_INDEX) {
		error("Failed to rescale font atlas as texture is not valid");
		return;
	}

	auto &texture = render_registry.textures[atlas->texture_index];
	assert(texture.flags & TEXTURE_FLAG_2D_ARRAY);

	texture.width = font_size;
	texture.height = font_size;

	const Font *font = atlas->font;
	atlas->font_size = font_size;

	const u32 charcode_count = atlas->end_charcode - atlas->start_charcode + 1;
	const f32 scale = stbtt_ScaleForPixelHeight(font->info, (f32)font_size);
	atlas->px_h_scale = scale;
	atlas->line_height = (s32)((font->ascent - font->descent + font->line_gap) * scale);

	const s32 space_glyph_index = stbtt_FindGlyphIndex(font->info, ' ');
	stbtt_GetGlyphHMetrics(font->info, space_glyph_index, &atlas->space_advance_width, 0);
	atlas->space_advance_width = (s32)(atlas->space_advance_width * scale);

	s32 max_layers;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
	assert(charcode_count <= (u32)max_layers);

	// stbtt rasterizes glyphs as 8bpp, so tell open gl to use 1 byte per color channel.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D_ARRAY, texture.id);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, atlas->font_size, atlas->font_size, charcode_count, 0, GL_RED, GL_UNSIGNED_BYTE, null);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	u8 *bitmap = (u8 *)push(temp, font_size * font_size);
	for (u32 i = 0; i < charcode_count; ++i) {
		const u32 c = i + atlas->start_charcode;
		Font_Glyph_Metric *metric = atlas->metrics + i;

		s32 w, h, offx, offy;
		const s32 glyph_index = stbtt_FindGlyphIndex(font->info, c);
		u8 *stb_bitmap = stbtt_GetGlyphBitmap(font->info, scale, scale, glyph_index, &w, &h, &offx, &offy);

		// Offset original bitmap to be at center of new one.
		const s32 x_offset = (font_size - w) / 2;
		const s32 y_offset = (font_size - h) / 2;

		metric->offset_x = offx - x_offset;
		metric->offset_y = offy - y_offset;

		memset(bitmap, 0, font_size * font_size);

		// @Cleanup: looks nasty, should come up with better solution.
		// Now we bake bitmap using stbtt and 'rescale' it to font size square one.
		// Maybe use stbtt_MakeGlyphBitmap at least?
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				int src_index = y * w + x;
				int dest_index = (y + y_offset) * font_size + (x + x_offset);
				if (dest_index >= 0 && dest_index < font_size * font_size)
					bitmap[dest_index] = stb_bitmap[src_index];
			}
		}

		stbtt_FreeBitmap(stb_bitmap, null);

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, font_size, font_size, 1, GL_RED, GL_UNSIGNED_BYTE, bitmap);

		s32 advance_width = 0;
		stbtt_GetGlyphHMetrics(font->info, glyph_index, &advance_width, 0);
		metric->advance_width = (s32)(advance_width * scale);
	}

	pop(temp, font_size * font_size);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default color channel size
}
