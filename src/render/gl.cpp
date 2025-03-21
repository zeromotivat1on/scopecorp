#include "pch.h"
#include "render/gfx.h"
#include "render/glad.h"
#include "render/draw.h"
#include "render/text.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"
#include "render/frame_buffer.h"
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

#include <malloc.h>
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

    if (flags & GFX_FLAG_STENCIL) {
        glEnable(GL_STENCIL_TEST);
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

void clear_screen(vec3 color, u32 flags) {
	glClearColor(color.x, color.y, color.z, 1.0f);

    if (flags & CLEAR_FLAG_COLOR)   glClear(GL_COLOR_BUFFER_BIT);
    if (flags & CLEAR_FLAG_DEPTH)   glClear(GL_DEPTH_BUFFER_BIT);
    if (flags & CLEAR_FLAG_STENCIL) glClear(GL_STENCIL_BUFFER_BIT);
}

static s32 gl_draw_mode(Draw_Mode mode) {
	switch (mode) {
	case DRAW_TRIANGLES:      return GL_TRIANGLES;
	case DRAW_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
	case DRAW_LINES:          return GL_LINES;
	default:
		error("Failed to get GL draw mode from given draw mode %d", mode);
		return -1;
	}
}

static s32 gl_texture_type(Texture_Type type) {
    switch (type) {
    case TEXTURE_TYPE_2D:       return GL_TEXTURE_2D;
    case TEXTURE_TYPE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;
    default:
        error("Failed to get GL texture type from type %d", type);
        return -1;
    }
}

void draw(const Draw_Command *command) {
	if (command->flags & DRAW_FLAG_SKIP_DRAW) return;
	if (command->flags & DRAW_FLAG_IGNORE_DEPTH) glDepthMask(GL_FALSE);

    if (command->flags & DRAW_FLAG_WIREFRAME) {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (command->flags & DRAW_FLAG_OUTLINE) {
        
    }
    
	const s32 draw_mode = gl_draw_mode(command->draw_mode);

    const auto &material = render_registry.materials[command->material_index];
    const auto &shader   = render_registry.shaders[material.shader_index];
    glUseProgram(shader.id);
     
	for (s32 i = 0; i < material.uniform_count; ++i)
		sync_uniform(material.uniforms + i);

	if (material.texture_index != INVALID_INDEX) {
		const auto &texture = render_registry.textures[material.texture_index];
        const s32 gl_type = gl_texture_type(texture.type);
		glBindTexture(gl_type, texture.id);
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

    if (viewport->frame_buffer_index != INVALID_INDEX) {
        recreate_frame_buffer(viewport->frame_buffer_index, viewport->width, viewport->height);
        log("Recreated viewport frame buffer %dx%d", viewport->width, viewport->height);
    }
}

s32 create_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_attachment_formats, s32 color_attachment_count, Texture_Format_Type depth_attachment_format) {
    assert(color_attachment_count <= MAX_FRAME_BUFFER_COLOR_ATTACHMENTS);
    
    Frame_Buffer frame_buffer;
    glGenFramebuffers(1, &frame_buffer.id);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);

    for (s32 i = 0; i < MAX_FRAME_BUFFER_COLOR_ATTACHMENTS; ++i)
        frame_buffer.color_attachments[i] = INVALID_INDEX;

    u32 *gl_attachments = (u32 *)alloca(color_attachment_count * sizeof(u32));
    for (s32 i = 0; i < color_attachment_count; ++i)
        gl_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        
    glDrawBuffers(color_attachment_count, gl_attachments);
    
    frame_buffer.color_attachment_count = color_attachment_count;
    memcpy(frame_buffer.color_attachment_formats, color_attachment_formats, color_attachment_count * sizeof(Texture_Format_Type));

    frame_buffer.depth_attachment        = INVALID_INDEX;
    frame_buffer.depth_attachment_format = depth_attachment_format;
    
    const s32 fbi = render_registry.frame_buffers.add(frame_buffer);
    recreate_frame_buffer(fbi, width, height);
    
    return fbi;
}

void recreate_frame_buffer(s32 fbi, s16 width, s16 height) {
    auto &frame_buffer = render_registry.frame_buffers[fbi];

    frame_buffer.width = width;
    frame_buffer.height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    
    for (s32 i = 0; i < frame_buffer.color_attachment_count; ++i) {
        s32 attachment = frame_buffer.color_attachments[i];
        if (attachment != INVALID_INDEX) delete_texture(attachment);

        const auto format = frame_buffer.color_attachment_formats[i];
        attachment = create_texture(TEXTURE_TYPE_2D, format, width, height, null);
        set_texture_filter(attachment, TEXTURE_FILTER_LINEAR);
        
        const auto &texture = render_registry.textures[attachment];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture.id, 0);
        
        frame_buffer.color_attachments[i] = attachment;
    }

    {
        s32 attachment = frame_buffer.depth_attachment;
        if (attachment != INVALID_INDEX) delete_texture(attachment);

        const auto format = frame_buffer.depth_attachment_format;
        attachment = create_texture(TEXTURE_TYPE_2D, format, width, height, null);
        set_texture_filter(attachment, TEXTURE_FILTER_LINEAR);
        
        const auto &texture = render_registry.textures[attachment];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0);
        
        frame_buffer.depth_attachment = attachment;
    }
    
    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        warn("Framebuffer %d is not complete, status 0x%X", fbi, status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

s32 read_frame_buffer_pixel(s32 fbi, s32 color_attachment_index, s32 x, s32 y) {
    const auto &frame_buffer = render_registry.frame_buffers[fbi];
    assert(color_attachment_index < frame_buffer.color_attachment_count);
    
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    s32 pixel = -1;
    y = viewport.height - y - 1; // invert as gl framebuffer y goes up, but mouse y goes down
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel);
    
    return pixel;
}

void start_frame_buffer_draw(s32 fbi) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_registry.frame_buffers[fbi].id);
}

void end_frame_buffer_draw() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static s32 create_quad_vertex_buffer() {
    struct Vertex_Quad { vec2 pos; vec2 uv; };
    Vertex_Quad vertices[] = {
        { vec2( 1.0f,  1.0f), vec2(1.0f, 1.0f) },
        { vec2( 1.0f, -1.0f), vec2(1.0f, 0.0f) },
        { vec2(-1.0f, -1.0f), vec2(0.0f, 0.0f) },
        { vec2(-1.0f,  1.0f), vec2(0.0f, 1.0f) },
    };
    Vertex_Component_Type components[] = { VERTEX_F32_2, VERTEX_F32_2 };
    return create_vertex_buffer(components, c_array_count(components), (f32 *)vertices, c_array_count(vertices), BUFFER_USAGE_STATIC);    
}

static s32 create_quad_index_buffer() {
    u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
    return create_index_buffer(indices, c_array_count(indices), BUFFER_USAGE_STATIC);
}

void draw_frame_buffer(s32 fbi, s32 color_attachment_index) {
    static s32 vertex_buffer_index = create_quad_vertex_buffer();
    static s32 index_buffer_index  = create_quad_index_buffer();
    static s32 material_index      = render_registry.materials.add(Material(shader_index_list.frame_buffer, render_registry.frame_buffers[fbi].color_attachments[color_attachment_index]));
    
    glDisable(GL_DEPTH_TEST);
        
    clear_screen(vec3_red, CLEAR_FLAG_COLOR);

    // Draw frame buffer screen quad texture we've rendered on earlier.
    Draw_Command command;
    command.flags = DRAW_FLAG_ENTIRE_BUFFER;
    command.draw_mode = DRAW_TRIANGLES;
    command.vertex_buffer_index = vertex_buffer_index;
    command.index_buffer_index  = index_buffer_index;
    command.material_index = material_index;
    draw(&command);

    glEnable(GL_DEPTH_TEST);
}

static s32 gl_usage(Buffer_Usage_Type type) {
	switch (type) {
	case BUFFER_USAGE_STATIC:  return GL_STATIC_DRAW;
	case BUFFER_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW;
	case BUFFER_USAGE_STREAM:  return GL_STREAM_DRAW;
	default:
		error("Failed to get GL usage from given buffer usage %d", type);
		return -1;
	}
}

static s32 gl_vertex_data_type(Vertex_Component_Type type) {
    switch (type) {
	case VERTEX_S32:   return GL_INT;
	case VERTEX_U32:   return GL_UNSIGNED_INT;
	case VERTEX_F32_2: return GL_FLOAT;
	case VERTEX_F32_3: return GL_FLOAT;
	case VERTEX_F32_4: return GL_FLOAT;
    default:
        error("Failed to get vertex data type from vertex component type %d", type);
        return -1;
    }
}

s32 create_vertex_buffer(const Vertex_Component_Type *components, s32 component_count, const void *data, s32 vertex_count, Buffer_Usage_Type usage_type) {
	assert(component_count <= MAX_VERTEX_LAYOUT_SIZE);

	Vertex_Buffer buffer = {0};
	buffer.usage_type = usage_type;
	buffer.vertex_count = vertex_count;
    buffer.layout.component_count = component_count;
	memcpy(&buffer.layout.components, components, component_count * sizeof(Vertex_Component_Type));

	glGenBuffers(1, &buffer.handle);
	glGenVertexArrays(1, &buffer.id);

	glBindVertexArray(buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.handle);

	s32 vertex_size = 0;
	for (s32 i = 0; i < component_count; ++i)
		vertex_size += vertex_component_size(components[i]);

    const s32 usage = gl_usage(usage_type);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * vertex_size, data, usage);

	for (s32 i = 0; i < component_count; ++i) {
		const s32 dimension = vertex_component_dimension(components[i]);

		s32 offset = 0;
		for (s32 j = 0; j < i; ++j)
			offset += vertex_component_size(components[j]);

        const s32 data_type = gl_vertex_data_type(components[i]);
		glEnableVertexAttribArray(i);

        if (components[i] == VERTEX_S32 || components[i] == VERTEX_U32)
            glVertexAttribIPointer(i, dimension, data_type, vertex_size, (void *)(u64)offset);
        else
            glVertexAttribPointer(i, dimension, data_type, GL_FALSE, vertex_size, (void *)(u64)offset);
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
	case UNIFORM_F32_2:
		glUniform2fv(uniform->location, uniform->count, (f32 *)uniform->value);
		break;
	case UNIFORM_F32_3:
		glUniform3fv(uniform->location, uniform->count, (f32 *)uniform->value);
		break;
	case UNIFORM_F32_4X4:
		glUniformMatrix4fv(uniform->location, uniform->count, GL_FALSE, (f32 *)uniform->value);
		break;
	default:
		error("Failed to sync uniform of unknown type %d", uniform->type);
		break;
	}
}

s32 create_texture(const char *path) {
	char timer_string[256];
	stbsp_snprintf(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
	SCOPE_TIMER(timer_string);

	stbi_set_flip_vertically_on_load(true);
	defer { stbi_set_flip_vertically_on_load(false); };

	u64 buffer_size = 0;
	u8 *buffer = (u8 *)push(temp, MAX_TEXTURE_SIZE);
	defer { pop(temp, MAX_TEXTURE_SIZE); };

	if (!read_file(path, buffer, MAX_TEXTURE_SIZE, &buffer_size)) {
		return INVALID_INDEX;
	}

    s32 width, height;
	u8 *data = stbi_load_from_memory(buffer, (s32)buffer_size, &width, &height, null, 4);
	if (!data) {
		error("Failed to load texture %s, stbi reason %s", path, stbi_failure_reason());
		return INVALID_INDEX;
	}

    const s32 texture = create_texture(TEXTURE_TYPE_2D, TEXTURE_FORMAT_RGBA_8, width, height, data);
    generate_texture_mipmaps(texture);
    set_texture_wrap(texture, TEXTURE_WRAP_REPEAT);
    set_texture_filter(texture, TEXTURE_FILTER_NEAREST);
    
	return texture;
}

static s32 gl_texture_format(Texture_Format_Type format) {
    switch (format) {
    case TEXTURE_FORMAT_RED_INTEGER:        return GL_RED_INTEGER;
    case TEXTURE_FORMAT_RGB_8:              return GL_RGB;
    case TEXTURE_FORMAT_RGBA_8:             return GL_RGBA;
    case TEXTURE_FORMAT_DEPTH_24_STENCIL_8: return GL_DEPTH_STENCIL;
    default:
        error("Failed to get GL texture format from format %d", format);
        return -1;
    }
}

static s32 gl_texture_internal_format(Texture_Format_Type format) {
    switch (format) {
    case TEXTURE_FORMAT_RED_INTEGER:        return GL_R32I;
    case TEXTURE_FORMAT_RGB_8:              return GL_RGB8;
    case TEXTURE_FORMAT_RGBA_8:             return GL_RGBA8;
    case TEXTURE_FORMAT_DEPTH_24_STENCIL_8: return GL_DEPTH24_STENCIL8;
    default:
        error("Failed to get GL texture internal format from format %d", format);
        return -1;
    }
}

static s32 gl_texture_data_type(Texture_Format_Type format) {
    switch (format) {
        //case TEXTURE_FORMAT_RED_INTEGER:        return GL_INT32;
    case TEXTURE_FORMAT_RED_INTEGER:        return GL_INT;
    case TEXTURE_FORMAT_RGB_8:              return GL_UNSIGNED_BYTE;
    case TEXTURE_FORMAT_RGBA_8:             return GL_UNSIGNED_BYTE;
    case TEXTURE_FORMAT_DEPTH_24_STENCIL_8: return GL_UNSIGNED_INT_24_8;
    default:
        error("Failed to get GL texture data type from format %d", format);
        return -1;
    }
}

s32 create_texture(Texture_Type type, Texture_Format_Type format, s32 width, s32 height, void *data) {
    Texture texture = {0};
    texture.type = type;
    texture.format = format;
    texture.width = width;
    texture.height = height;

    glGenTextures(1, &texture.id);

    const s32 gl_type            = gl_texture_type(type);
    const s32 gl_format          = gl_texture_format(format);
    const s32 gl_internal_format = gl_texture_internal_format(format);
    const s32 gl_data_type       = gl_texture_data_type(format);

    if (gl_type < 0 || gl_format < 0 || gl_internal_format < 0 || gl_data_type < 0) {
        error("Failed to create texture, see errors above");
        return INVALID_INDEX;
    }
    
	glBindTexture(gl_type, texture.id);

    // @Todo: properly set data based on texture type.
    glTexImage2D(gl_type, 0, gl_internal_format, width, height, 0, gl_format, gl_data_type, data);
    
	glBindTexture(gl_type, 0);
    
    return render_registry.textures.add(texture);
}

static s32 gl_texture_wrap(Texture_Wrap_Type wrap) {
    switch (wrap) {
    case TEXTURE_WRAP_REPEAT: return GL_REPEAT;
    case TEXTURE_WRAP_CLAMP:  return GL_CLAMP_TO_EDGE;    
    default:                  return -1;
    }
}

void set_texture_wrap(s32 texture_index, Texture_Wrap_Type wrap) {
    auto &texture = render_registry.textures[texture_index];
    texture.wrap = wrap;
    
    const s32 gl_type = gl_texture_type(texture.type);
    const s32 gl_wrap = gl_texture_wrap(wrap);

    glBindTexture(gl_type, texture.id);
    glTexParameteri(gl_type, GL_TEXTURE_WRAP_S, gl_wrap);
    glTexParameteri(gl_type, GL_TEXTURE_WRAP_T, gl_wrap);
    glBindTexture(gl_type, 0);
}

static s32 gl_texture_min_filter(u32 filter, bool has_mipmaps) {
    switch (filter) {
    case TEXTURE_FILTER_LINEAR:
        if (has_mipmaps) return GL_LINEAR_MIPMAP_LINEAR;
        else             return GL_LINEAR;
    case TEXTURE_FILTER_NEAREST:
        if (has_mipmaps) return GL_NEAREST_MIPMAP_LINEAR;
        else             return GL_NEAREST;
    default:
        return -1;
    }
}

static s32 gl_texture_mag_filter(Texture_Filter_Type filter) {
    switch (filter) {
    case TEXTURE_FILTER_LINEAR:  return GL_LINEAR;
    case TEXTURE_FILTER_NEAREST: return GL_NEAREST;
    default:                     return -1;
    }
}

void set_texture_filter(s32 texture_index, Texture_Filter_Type filter) {
    auto &texture = render_registry.textures[texture_index];
    texture.filter = filter;

    const bool has_mipmaps = texture.flags & TEXTURE_HAS_MIPMAPS;
    
    const s32 gl_type = gl_texture_type(texture.type);
    const s32 gl_min_filter = gl_texture_min_filter(filter, has_mipmaps);
    const s32 gl_mag_filter = gl_texture_mag_filter(filter);

    glBindTexture(gl_type, texture.id);
    glTexParameteri(gl_type, GL_TEXTURE_MIN_FILTER, gl_min_filter);
    glTexParameteri(gl_type, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
    glBindTexture(gl_type, 0);
}

void generate_texture_mipmaps(s32 texture_index) {
    auto &texture = render_registry.textures[texture_index];
    texture.flags |= TEXTURE_HAS_MIPMAPS;

    const s32 gl_type = gl_texture_type(texture.type);
    glBindTexture(gl_type, texture.id);
    glGenerateMipmap(gl_type);
    glBindTexture(gl_type, 0);
}

void delete_texture(s32 texture_index) {
    auto *texture = render_registry.textures.find(texture_index);
    if (!texture) {
        warn("Failed to delete invalid texture %d", texture_index);
        return;
    }

    glDeleteTextures(1, &texture->id);
    render_registry.textures.remove(texture_index);
}

Font_Atlas *bake_font_atlas(const Font *font, u32 start_charcode, u32 end_charcode, s16 font_size) {
	Font_Atlas *atlas = push_struct(pers, Font_Atlas);
	atlas->font = font;
	atlas->texture_index = INVALID_INDEX;

	const u32 charcode_count = end_charcode - start_charcode + 1;
	atlas->metrics = push_array(pers, charcode_count, Font_Glyph_Metric);
	atlas->start_charcode = start_charcode;
	atlas->end_charcode = end_charcode;

	//Texture texture = {0};
	//glGenTextures(1, &texture.id);
	//texture.flags = TEXTURE_TYPE_2D_ARRAY | TEXTURE_FORMAT_RGBA_8;
	//texture.width = font_size;
	//texture.height = font_size;
	//texture.path = "texture for glyph batch rendering";

	//atlas->texture_index = render_registry.textures.add(texture);
	atlas->texture_index = create_texture(TEXTURE_TYPE_2D_ARRAY, TEXTURE_FORMAT_RGBA_8, font_size, font_size, null);

	rescale_font_atlas(atlas, font_size);

	return atlas;
}

void rescale_font_atlas(Font_Atlas *atlas, s16 font_size) {
	if (atlas->texture_index == INVALID_INDEX) {
		error("Failed to rescale font atlas with invalid texture");
		return;
	}

	auto &texture = render_registry.textures[atlas->texture_index];
	assert(texture.type == TEXTURE_TYPE_2D_ARRAY);

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

	// stbtt rasterizes glyphs as 8bpp, so tell GL to use 1 byte per color channel.
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
