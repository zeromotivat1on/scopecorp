#include "pch.h"
#include "render/render_init.h"
#include "render/glad.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"
#include "render/frame_buffer.h"
#include "render/index_buffer.h"
#include "render/vertex_buffer.h"
#include "render/render_command.h"
#include "render/render_registry.h"
#include "render/viewport.h"

#include "log.h"
#include "str.h"
#include "font.h"
#include "asset.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_sprintf.h"

#include "os/file.h"

#include "math/math_core.h"

#define gl_check_error() {                          \
        GLenum gl_error = glGetError();             \
        while (gl_error != GL_NO_ERROR) {           \
            error("OpenGL error 0x%X at %s:%d",     \
                  gl_error, __FILE__, __LINE__);    \
            gl_error = glGetError();                \
        }                                           \
    }

void detect_render_capabilities() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &R_MAX_TEXTURE_TEXELS);
    
    R_UNIFORM_BUFFER_BASE_ALIGNMENT = 16;    
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,     &R_MAX_UNIFORM_BUFFER_BINDINGS);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,          &R_MAX_UNIFORM_BLOCK_SIZE);

    log("Render capabilities:");
    log("  Texture: Max Size %dx%d texels", R_MAX_TEXTURE_TEXELS, R_MAX_TEXTURE_TEXELS);
    log("  Uniform buffer: Max Bindings %d | Base Alignment %d | Offset Alignment %d",
        R_MAX_UNIFORM_BUFFER_BINDINGS,
        R_UNIFORM_BUFFER_BASE_ALIGNMENT,
        R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    log("  Uniform block:  Max Size %d bytes", R_MAX_UNIFORM_BLOCK_SIZE);
}

static u32 gl_clear_buffer_bits(u32 flags) {
    u32 bits = 0;
    if (flags & CLEAR_FLAG_COLOR)   bits |= GL_COLOR_BUFFER_BIT;
    if (flags & CLEAR_FLAG_DEPTH)   bits |= GL_DEPTH_BUFFER_BIT;
    if (flags & CLEAR_FLAG_STENCIL) bits |= GL_STENCIL_BUFFER_BIT;
    return bits;
}

static s32 gl_draw_mode(Render_Mode mode) {
	switch (mode) {
	case RENDER_TRIANGLES:      return GL_TRIANGLES;
	case RENDER_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
	case RENDER_LINES:          return GL_LINES;
	default:
		error("Failed to get GL draw mode from given mode %d", mode);
		return -1;
	}
}

static s32 gl_polygon_mode(Polygon_Mode mode) {
	switch (mode) {
	case POLYGON_FILL:  return GL_FILL;
	case POLYGON_LINE:  return GL_LINE;
	case POLYGON_POINT: return GL_POINT;
	default:
		error("Failed to get GL polygon mode from given mode %d", mode);
		return -1;
	}
}

static s32 gl_winding(Winding_Type type) {
	switch (type) {
	case WINDING_CLOCKWISE:         return GL_CW;
	case WINDING_COUNTER_CLOCKWISE: return GL_CCW;
	default:
		error("Failed to get GL winding type from type %d", type);
		return -1;
	}
}

static s32 gl_blend_function(Blend_Test_Function_Type function) {
	switch (function) {
	case BLEND_SOURCE_ALPHA:           return GL_SRC_ALPHA;
	case BLEND_ONE_MINUS_SOURCE_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	default:
		error("Failed to get GL winding function from function %d", function);
		return -1;
	}
}

static s32 gl_cull_face(Cull_Face_Type type) {
	switch (type) {
	case CULL_FACE_BACK:  return GL_BACK;
	case CULL_FACE_FRONT: return GL_FRONT;
	default:
		error("Failed to get GL cull face type from type %d", type);
		return -1;
	}
}

static s32 gl_depth_function(Depth_Test_Function_Type function) {
	switch (function) {
	case DEPTH_LESS: return GL_LESS;
	default:
		error("Failed to get GL depth test function from function %d", function);
		return -1;
	}
}

static s32 gl_depth_mask(Depth_Test_Mask_Type mask) {
	switch (mask) {
	case DEPTH_ENABLE:  return GL_TRUE;
	case DEPTH_DISABLE: return GL_FALSE;
	default:
		error("Failed to get GL depth test mask from mask %d", mask);
		return -1;
	}
}

static s32 gl_stencil_operation(Stencil_Test_Operation_Type operation) {
	switch (operation) {
	case STENCIL_KEEP:    return GL_KEEP;
	case STENCIL_REPLACE: return GL_REPLACE;
	default:
		error("Failed to get GL stencil test operation from operation %d", operation);
		return -1;
	}
}

static s32 gl_stencil_function(Stencil_Test_Function_Type function) {
	switch (function) {
	case STENCIL_ALWAYS:    return GL_ALWAYS;
	case STENCIL_EQUAL:     return GL_EQUAL;
	case STENCIL_NOT_EQUAL: return GL_NOTEQUAL;
	default:
		error("Failed to get GL stencil test function from function %d", function);
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

void submit(const Render_Command *command) {    
    {
        const s32 mode = gl_polygon_mode(command->polygon_mode);
        glPolygonMode(GL_FRONT_AND_BACK, mode);
    }

    if (command->flags & RENDER_FLAG_VIEWPORT) {
        const auto &viewport = command->viewport;
        glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    }
    
    if (command->flags & RENDER_FLAG_SCISSOR) {
        const auto &scissor = command->scissor;
        
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissor.x, scissor.y, scissor.width, scissor.height);
    }

    if (command->flags & RENDER_FLAG_CLEAR) {
        const auto &clear = command->clear;
        const u32 bits = gl_clear_buffer_bits(clear.flags);

        glDepthMask(GL_TRUE);
        glClearColor(clear.color[0], clear.color[1], clear.color[2], 1.0f);
        glClear(bits);
        glDepthMask(GL_FALSE);
    }
    
    if (command->flags & RENDER_FLAG_CULL_FACE) {
        const auto &cull_face = command->cull_face;

        const s32 face    = gl_cull_face(cull_face.type);
        const s32 winding = gl_winding(cull_face.winding);
        
        glEnable(GL_CULL_FACE);
		glCullFace(face);
        glFrontFace(winding);
    }

    if (command->flags & RENDER_FLAG_BLEND) {
        const auto &blend = command->blend;
        
        const s32 source      = gl_blend_function(blend.source);
        const s32 destination = gl_blend_function(blend.destination);
        
        glEnable(GL_BLEND);
        glBlendFunc(source, destination);
    }
    
    if (command->flags & RENDER_FLAG_DEPTH) {
        const auto &depth = command->depth;

        const s32 function = gl_depth_function(depth.function);
        const s32 mask     = gl_depth_mask(depth.mask);
        
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(function);
        glDepthMask(mask);
    }

    if (command->flags & RENDER_FLAG_STENCIL) {
        const auto &stencil = command->stencil;
        
        const s32 stencil_failed = gl_stencil_operation(stencil.operation.stencil_failed);
        const s32 depth_failed   = gl_stencil_operation(stencil.operation.depth_failed);
        const s32 both_passed    = gl_stencil_operation(stencil.operation.both_passed);
        const s32 function_type  = gl_stencil_function(stencil.function.type);
        
        glEnable(GL_STENCIL_TEST);
        glStencilOp(stencil_failed, depth_failed, both_passed);
        glStencilFunc(function_type, stencil.function.comparator, stencil.function.mask);
        glStencilMask(command->stencil.mask);
    }
    
    if (command->frame_buffer_index != INVALID_INDEX) {
        const auto &frame_buffer = render_registry.frame_buffers[command->frame_buffer_index];
        glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    }

    if (command->shader_index != INVALID_INDEX) {
        const auto &shader = render_registry.shaders[command->shader_index];
        glUseProgram(shader.id);
        
        for (s32 i = 0; i < command->uniform_count; ++i) {
            send_uniform_value_to_gpu(command->shader_index,
                                      command->uniform_indices[i],
                                      command->uniform_value_offsets[i]);
        }
    }

	if (command->texture_index != INVALID_INDEX) {
		const auto &texture = render_registry.textures[command->texture_index];
        const s32 type      = gl_texture_type(texture.type);
        
		glBindTexture(type, texture.id);
		glActiveTexture(GL_TEXTURE0);
	}

    if (command->vertex_array_index != INVALID_INDEX) {
        const auto &vertex_array = render_registry.vertex_arrays[command->vertex_array_index];
        glBindVertexArray(vertex_array.id);

        const s32 render_mode = gl_draw_mode(command->render_mode);
        
        if (command->index_buffer_index != INVALID_INDEX) {
            const auto &index_buffer = render_registry.index_buffers[command->index_buffer_index];
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer.id);

            const s32 index_count = command->buffer_element_count;
            const void* index_ptr = (void *)(u64)command->buffer_element_offset;
            glDrawElementsInstanced(render_mode, index_count, GL_UNSIGNED_INT, index_ptr, command->instance_count);
        } else {
            const s32 vertex_count = command->buffer_element_count;
            const s32 vertex_first = command->buffer_element_offset;
            glDrawArraysInstanced(render_mode, vertex_first, vertex_count, command->instance_count);
        }
    }
    
    if (command->flags & RENDER_FLAG_RESET) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        if (command->flags & RENDER_FLAG_VIEWPORT) {
            glViewport(0, 0, 0, 0);
        }
        
        if (command->flags & RENDER_FLAG_SCISSOR) {
            glDisable(GL_SCISSOR_TEST);
            glScissor(0, 0, 0, 0);
        }

        if (command->flags & RENDER_FLAG_CULL_FACE) {
            glDisable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
        }
        
        if (command->flags & RENDER_FLAG_BLEND) {
            glDisable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        if (command->flags & RENDER_FLAG_DEPTH) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
        }

        if (command->flags & RENDER_FLAG_STENCIL) {
            glDisable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
        }

        if (command->frame_buffer_index != INVALID_INDEX) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        if (command->texture_index != INVALID_INDEX) {
            const auto &texture = render_registry.textures[command->texture_index];
            const s32 type      = gl_texture_type(texture.type);
            glBindTexture(type, 0);
        }

        if (command->index_buffer_index != INVALID_INDEX) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        if (command->vertex_array_index != INVALID_INDEX) {
            glBindVertexArray(0);
        }

        if (command->shader_index != INVALID_INDEX) {
            glUseProgram(0);
        }
    }
}

s32 create_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_attachments, s32 color_attachment_count, Texture_Format_Type depth_attachment) {
    Assert(color_attachment_count <= MAX_FRAME_BUFFER_COLOR_ATTACHMENTS);
    
    Frame_Buffer frame_buffer = {};
    glGenFramebuffers(1, &frame_buffer.id);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);

    for (s32 i = 0; i < MAX_FRAME_BUFFER_COLOR_ATTACHMENTS; ++i)
        frame_buffer.color_attachment_indices[i] = INVALID_INDEX;

    u32 gl_attachments[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    for (s32 i = 0; i < color_attachment_count; ++i)
        gl_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        
    glDrawBuffers(color_attachment_count, gl_attachments);
    
    frame_buffer.color_attachment_count = color_attachment_count;
    copy_bytes(frame_buffer.color_attachment_formats, color_attachments, color_attachment_count * sizeof(Texture_Format_Type));

    frame_buffer.depth_attachment_index  = INVALID_INDEX;
    frame_buffer.depth_attachment_format = depth_attachment;

    frame_buffer.material_index = create_material(asset_table[shader_sids.frame_buffer].registry_index, INVALID_INDEX);

    const s32 uniforms[] = {
        create_uniform("u_resolution",                  UNIFORM_F32_2, 1),
        create_uniform("u_pixel_size",                  UNIFORM_F32,   1),
        create_uniform("u_curve_distortion_factor",     UNIFORM_F32,   1),
        create_uniform("u_chromatic_aberration_offset", UNIFORM_F32,   1),
        create_uniform("u_quantize_color_count",        UNIFORM_U32,   1),
        create_uniform("u_noise_blend_factor",          UNIFORM_F32,   1),
        create_uniform("u_scanline_count",              UNIFORM_U32,   1),
        create_uniform("u_scanline_intensity",          UNIFORM_F32,   1),
    };
    
    set_material_uniforms(frame_buffer.material_index, uniforms, COUNT(uniforms));

    const vec2 resolution = vec2(width, height);
    set_material_uniform_value(frame_buffer.material_index, 0, &resolution);
    
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
        s32 attachment = frame_buffer.color_attachment_indices[i];
        if (attachment != INVALID_INDEX) delete_texture(attachment);

        const auto format = frame_buffer.color_attachment_formats[i];
        attachment = create_texture(TEXTURE_TYPE_2D, format, width, height, null);
        set_texture_filter(attachment, TEXTURE_FILTER_NEAREST);
        set_texture_wrap(attachment, TEXTURE_WRAP_BORDER);
        
        const auto &texture = render_registry.textures[attachment];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture.id, 0);
        
        frame_buffer.color_attachment_indices[i] = attachment;
    }

    {
        s32 attachment = frame_buffer.depth_attachment_index;
        if (attachment != INVALID_INDEX) delete_texture(attachment);

        const auto format = frame_buffer.depth_attachment_format;
        attachment = create_texture(TEXTURE_TYPE_2D, format, width, height, null);
        set_texture_filter(attachment, TEXTURE_FILTER_NEAREST);
        
        const auto &texture = render_registry.textures[attachment];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture.id, 0);
        
        frame_buffer.depth_attachment_index = attachment;
    }
    
    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        warn("Framebuffer %d is not complete, status 0x%X", fbi, status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

s32 read_frame_buffer_pixel(s32 fbi, s32 color_attachment_index, s32 x, s32 y) {
    const auto &frame_buffer = render_registry.frame_buffers[fbi];
    Assert(color_attachment_index < frame_buffer.color_attachment_count);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    s32 pixel = -1;
    x = (s32)((f32)(x - viewport.x) * viewport.resolution_scale);
    y = (s32)((f32)(viewport.height - y - 1) * viewport.resolution_scale);
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return pixel;
}

static s32 create_frame_buffer_vertex_array() {
    struct Vertex_Quad { vec2 pos; vec2 uv; };
    const Vertex_Quad vertices[] = {
        { vec2( 1.0f,  1.0f), vec2(1.0f, 1.0f) },
        { vec2( 1.0f, -1.0f), vec2(1.0f, 0.0f) },
        { vec2(-1.0f, -1.0f), vec2(0.0f, 0.0f) },
        { vec2(-1.0f,  1.0f), vec2(0.0f, 1.0f) },
    };

    Vertex_Array_Binding binding = {};
    binding.layout_size = 2;
    binding.layout[0] = { VERTEX_F32_2, 0 };
    binding.layout[1] = { VERTEX_F32_2, 0 };
    binding.vertex_buffer_index = create_vertex_buffer(vertices, COUNT(vertices) * sizeof(Vertex_Quad), BUFFER_USAGE_STATIC);

    return create_vertex_array(&binding, 1);
}

static s32 create_frame_buffer_index_buffer() {
    const u32 indices[] = { 0, 2, 1, 2, 0, 3 };
    return create_index_buffer(indices, COUNT(indices), BUFFER_USAGE_STATIC);
}

void draw_frame_buffer(s32 fbi, s32 color_attachment_index) {
    static const s32 vertex_array_index = create_frame_buffer_vertex_array();
    static const s32 index_buffer_index = create_frame_buffer_index_buffer();
    
    const auto &frame_buffer = render_registry.frame_buffers[fbi];

    set_material_uniform_value(frame_buffer.material_index, 1, &frame_buffer.pixel_size);
    set_material_uniform_value(frame_buffer.material_index, 2, &frame_buffer.curve_distortion_factor);
    set_material_uniform_value(frame_buffer.material_index, 3, &frame_buffer.chromatic_aberration_offset);
    set_material_uniform_value(frame_buffer.material_index, 4, &frame_buffer.quantize_color_count);
    set_material_uniform_value(frame_buffer.material_index, 5, &frame_buffer.noise_blend_factor);
    set_material_uniform_value(frame_buffer.material_index, 6, &frame_buffer.scanline_count);
    set_material_uniform_value(frame_buffer.material_index, 7, &frame_buffer.scanline_intensity);
    
    Render_Command command = {};
    command.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE;
    command.render_mode  = RENDER_TRIANGLES;
    command.polygon_mode = POLYGON_FILL;
    command.viewport.x      = viewport.x;
    command.viewport.y      = viewport.y;
    command.viewport.width  = viewport.width;
    command.viewport.height = viewport.height;
    command.scissor.x      = viewport.x;
    command.scissor.y      = viewport.y;
    command.scissor.width  = viewport.width;
    command.scissor.height = viewport.height;
    command.cull_face.type    = CULL_FACE_BACK;
    command.cull_face.winding = WINDING_COUNTER_CLOCKWISE;
    command.vertex_array_index = vertex_array_index;
    command.index_buffer_index = index_buffer_index;
    
    fill_render_command_with_material_data(frame_buffer.material_index, &command);
    command.texture_index = frame_buffer.color_attachment_indices[color_attachment_index];
    
    const auto &index_buffer = render_registry.index_buffers[index_buffer_index];
    command.buffer_element_count = index_buffer.index_count;
    
    submit(&command);
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

s32 create_vertex_buffer(const void *data, u32 size, Buffer_Usage_Type usage) {
	Vertex_Buffer buffer = {0};
	buffer.size  = size;
	buffer.usage = usage;

	glGenBuffers(1, &buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
	glBufferData(GL_ARRAY_BUFFER, size, data, gl_usage(usage));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return render_registry.vertex_buffers.add(buffer);
}

void set_vertex_buffer_data(s32 vertex_buffer_index, const void *data, u32 size, u32 offset) {
    const auto &buffer = render_registry.vertex_buffers[vertex_buffer_index];

	glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);    
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

s32 create_vertex_array(const Vertex_Array_Binding *bindings, s32 binding_count) {
    Vertex_Array array;
    array.binding_count = binding_count;
    copy_bytes(array.bindings, bindings, binding_count * sizeof(Vertex_Array_Binding));
    
    glGenVertexArrays(1, &array.id);
    glBindVertexArray(array.id);

    s32 binding_index = 0;
    for (s32 i = 0; i < binding_count; ++i) {
        Assert(binding_index < MAX_VERTEX_ARRAY_BINDINGS);
        
        const auto &binding = bindings[i];
        const auto &vertex_buffer = render_registry.vertex_buffers[binding.vertex_buffer_index];
        
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id);

        s32 vertex_size = 0;
        for (s32 j = 0; j < binding.layout_size; ++j)
            vertex_size += vertex_component_size(binding.layout[j].type);
    
        for (s32 j = 0; j < binding.layout_size; ++j) {
            const auto &component = binding.layout[j];
            
            s32 offset = 0;
            for (s32 k = 0; k < j; ++k)
                offset += vertex_component_size(binding.layout[k].type);

            const s32 data_type = gl_vertex_data_type(component.type);
            const s32 dimension = vertex_component_dimension(component.type);

            glEnableVertexAttribArray(binding_index);
            
            if (component.type == VERTEX_S32 || component.type == VERTEX_U32)
                glVertexAttribIPointer(binding_index, dimension, data_type, vertex_size, (void *)(u64)offset);
            else
                glVertexAttribPointer(binding_index, dimension, data_type, component.normalize, vertex_size, (void *)(u64)offset);
            
            glVertexAttribDivisor(binding_index, component.advance_rate);

            binding_index++;
        }
	}

    glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

    return render_registry.vertex_arrays.add(array);
}

s32 create_index_buffer(const u32 *indices, s32 count, Buffer_Usage_Type usage_type) {
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

s32 create_shader(const char *source, const char *path) {
	char *vertex_src   = (char *)allocl(MAX_SHADER_SIZE);
	char *fragment_src = (char *)allocl(MAX_SHADER_SIZE);
    defer { freel(2 * MAX_SHADER_SIZE); };

	if (!parse_shader(source, vertex_src, fragment_src)) {
        error("Failed to create shader %s", path);
        return INVALID_INDEX;
    }

	const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
	const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);

    Shader shader;
	shader.id = gl_link_program(vertex_shader, fragment_shader);

	return render_registry.shaders.add(shader);
}

bool recreate_shader(s32 shader_index, const char *source, const char *path) {
    auto &shader = render_registry.shaders[shader_index];

    char *vertex_src   = (char *)allocl(MAX_SHADER_SIZE);
	char *fragment_src = (char *)allocl(MAX_SHADER_SIZE);
    defer { freel(2 * MAX_SHADER_SIZE); };

	if (!parse_shader(source, vertex_src, fragment_src)) {
        error("Failed to recreate shader %s", path);
        return false;
    }
    
	// We are free to delete old shader program at this stage.
	glDeleteProgram(shader.id);

	const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
	const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
	shader.id = gl_link_program(vertex_shader, fragment_shader);

	return true;
}

const char *get_desired_shader_file_extension() {
    return ".glsl";
}

void send_uniform_value_to_gpu(s32 shader_index, s32 uniform_index, u32 offset) {
    const auto &cache = render_registry.uniform_value_cache;
    Assert(offset < cache.size);

    const auto &uniform = render_registry.uniforms[uniform_index];
    const auto &shader  = render_registry.shaders[shader_index];
    const s32 location  = glGetUniformLocation(shader.id, uniform.name);
    
    if (location < 0) {
        //error("Failed to get uniform %s location from shader %d", uniform.name, shader_index);
        return;
    }
    
    const void *data = (u8 *)cache.data + offset;
    
    switch (uniform.type) {
	case UNIFORM_U32:     glUniform1uiv(location, uniform.count, (u32 *)data); break;
    case UNIFORM_F32:     glUniform1fv(location,  uniform.count, (f32 *)data); break;
    case UNIFORM_F32_2:   glUniform2fv(location,  uniform.count, (f32 *)data); break;
    case UNIFORM_F32_3:   glUniform3fv(location,  uniform.count, (f32 *)data); break;
    case UNIFORM_F32_4:   glUniform4fv(location,  uniform.count, (f32 *)data); break;
	case UNIFORM_F32_4X4: glUniformMatrix4fv(location, uniform.count, GL_FALSE, (f32 *)data); break;
	default:
		error("Failed to sync uniform %s of unknown type %d", uniform.name, uniform.type);
		break;
	}
}

u32 get_uniform_type_size_gpu_aligned(Uniform_Type type) {
    constexpr u32 N = 4;
    
    switch (type) {
	case UNIFORM_U32:     return N;
	case UNIFORM_F32:     return N;
	case UNIFORM_F32_2:   return N * 2;
	case UNIFORM_F32_3:   return N * 4;
	case UNIFORM_F32_4:   return N * 4;
	case UNIFORM_F32_4X4: return N * 16;
    default:
        error("Failed to get uniform gpu aligned size from type %d", type);
        return 0;
    }
}

u32 get_uniform_type_alignment(Uniform_Type type) {
    constexpr u32 N = 4;

    switch (type) {
	case UNIFORM_U32:     return N;
	case UNIFORM_F32:     return N;
	case UNIFORM_F32_2:   return N * 2;
	case UNIFORM_F32_3:   return N * 4;
	case UNIFORM_F32_4:   return N * 4;
	case UNIFORM_F32_4X4: return N * 4;
    default:
        error("Failed to get uniform alignment from type %d", type);
        return 0;
    }    
}

s32 create_uniform_buffer(u32 size) {
    Uniform_Buffer buffer;
    buffer.size = size;

    glGenBuffers(1, &buffer.id);

    glBindBuffer(GL_UNIFORM_BUFFER, buffer.id);
    glBufferData(GL_UNIFORM_BUFFER, size, null, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    return render_registry.uniform_buffers.add(buffer);
}

// std140 alignment rules
static inline u32 get_uniform_block_field_offset_gpu_aligned(const Uniform_Block_Field *fields, s32 field_index, s32 field_element_index) {
    u32 offset = 0;
    for (s32 i = 0; i <= field_index; ++i) {
        const auto &field = fields[i];

        u32 stride = get_uniform_type_size_gpu_aligned(field.type);
        u32 alignment = get_uniform_type_alignment(field.type);
        
        if (field.count > 1) {
            alignment = R_UNIFORM_BUFFER_BASE_ALIGNMENT;
            stride = Align(stride, R_UNIFORM_BUFFER_BASE_ALIGNMENT);
        }

        offset = Align(offset, alignment);

        s32 element_count = field.count;
        if (i == field_index) {
            Assert(field_element_index < field.count);
            element_count = field_element_index;
        }
        
        offset += stride * element_count;
    }

    offset = Align(offset, R_UNIFORM_BUFFER_BASE_ALIGNMENT);

    return offset;
}

// std140 alignment rules
static inline u32 get_uniform_block_size_gpu_aligned(const Uniform_Block_Field *fields, s32 field_count) {
    u32 size = 0;
    for (s32 i = 0; i < field_count; ++i) {
        const auto &field = fields[i];

        u32 stride = get_uniform_type_size_gpu_aligned(field.type);
        u32 alignment = get_uniform_type_alignment(field.type);
        
        if (field.count > 1) {
            alignment = R_UNIFORM_BUFFER_BASE_ALIGNMENT;
            stride = Align(stride, R_UNIFORM_BUFFER_BASE_ALIGNMENT);
        }

        size = Align(size, alignment);
        size += stride * field.count;
    }

    size = Align(size, R_UNIFORM_BUFFER_BASE_ALIGNMENT);

    return size;
}

s32 create_uniform_block(s32 uniform_buffer_index, s32 shader_binding, const char *name, const Uniform_Block_Field *fields, s32 field_count) {
    Uniform_Block block;
    block.uniform_buffer_index = uniform_buffer_index;
    block.name = name;
    block.field_count = field_count;
    block.shader_binding = shader_binding;
    block.offset = 0;
    block.cpu_size = 0;
    block.gpu_size = get_uniform_block_size_gpu_aligned(fields, field_count);
    copy_bytes(block.fields, fields, field_count * sizeof(Uniform_Block_Field));
    
    For (render_registry.shaders) {
        const u32 gl_block_index = glGetUniformBlockIndex(it.id, name);
        if (gl_block_index != GL_INVALID_INDEX) {
            glUniformBlockBinding(it.id, gl_block_index, shader_binding);
        }
    }

    auto &buffer = render_registry.uniform_buffers[uniform_buffer_index];
    const u32 used_size = get_uniform_buffer_used_size_gpu_aligned(uniform_buffer_index);

    block.offset = Align(used_size, R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    Assert(block.offset + block.gpu_size < buffer.size);
    
    glBindBufferRange(GL_UNIFORM_BUFFER, shader_binding, buffer.id, block.offset, block.gpu_size);
    
    const s32 uniform_block_index = render_registry.uniform_blocks.add(block);
    
    Assert(buffer.block_count < MAX_UNIFORM_BUFFER_BLOCKS);
    buffer.block_indices[buffer.block_count] = uniform_block_index;
    buffer.block_count += 1;
    
    return uniform_block_index;
}

u32 get_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field) {
    if (field.count == 1) {
        return get_uniform_type_size_gpu_aligned(field.type);
    }

    return get_uniform_type_size_gpu_aligned(UNIFORM_F32_4) * field.count;
}

u32 get_uniform_block_field_offset_gpu_aligned(s32 uniform_block_index, s32 field_index, s32 field_element_index) {
    const auto &block = render_registry.uniform_blocks[uniform_block_index];
    Assert(field_index < block.field_count);
    Assert(field_element_index < block.fields[field_index].count);
    
    return get_uniform_block_field_offset_gpu_aligned(block.fields, field_index, field_element_index);
}

u32 get_uniform_block_size_gpu_aligned(s32 uniform_block_index) {
    const auto &block = render_registry.uniform_blocks[uniform_block_index];
    return get_uniform_block_size_gpu_aligned(block.fields, block.field_count);
}

void set_uniform_block_value(s32 ubbi, u32 offset, const void *data, u32 size) {
    auto &block = render_registry.uniform_blocks[ubbi];
    Assert(offset < block.gpu_size);

    const auto &uniform_buffer = render_registry.uniform_buffers[block.uniform_buffer_index];
    glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer.id);
    glBufferSubData(GL_UNIFORM_BUFFER, block.offset + offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void set_uniform_block_value(s32 ubbi, s32 field_index, s32 field_element_index, const void *data, u32 size) {
    const u32 field_offset = get_uniform_block_field_offset_gpu_aligned(ubbi, field_index, field_element_index);
    set_uniform_block_value(ubbi, field_offset, data, size);
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

s32 create_texture(Texture_Type texture_type, Texture_Format_Type format_type, s32 width, s32 height, void *data) {
    Texture texture = {0};
    texture.type = texture_type;
    texture.format = format_type;
    texture.width = width;
    texture.height = height;

    glGenTextures(1, &texture.id);

    const s32 type            = gl_texture_type(texture_type);
    const s32 format          = gl_texture_format(format_type);
    const s32 internal_format = gl_texture_internal_format(format_type);
    const s32 data_type       = gl_texture_data_type(format_type);
    
    if (type < 0 || format < 0 || internal_format < 0 || data_type < 0) {
        error("Failed to create texture, see errors above");
        return INVALID_INDEX;
    }
    
	glBindTexture(type, texture.id);

    // @Todo: properly set data based on texture type.
    glTexImage2D(type, 0, internal_format, width, height, 0, format, data_type, data);
    
	glBindTexture(type, 0);
    
    return render_registry.textures.add(texture);
}

bool recreate_texture(s32 texture_index, Texture_Type texture_type, Texture_Format_Type format_type, s32 width, s32 height, void *data) {
    auto &texture = render_registry.textures[texture_index];
    
    const s32 type            = gl_texture_type(texture_type);
    const s32 format          = gl_texture_format(format_type);
    const s32 internal_format = gl_texture_internal_format(format_type);
    const s32 data_type       = gl_texture_data_type(format_type);
    
    if (type < 0 || format < 0 || internal_format < 0 || data_type < 0) {
        error("Failed to create texture, see errors above");
        return false;
    }
    
	glBindTexture(type, texture.id);

    // @Todo: properly set data based on texture type.
    glTexImage2D(type, 0, internal_format, width, height, 0, format, data_type, data);
    
	glBindTexture(type, 0);

    return true;
}

static s32 gl_texture_wrap(Texture_Wrap_Type wrap) {
    switch (wrap) {
    case TEXTURE_WRAP_REPEAT:  return GL_REPEAT;
    case TEXTURE_WRAP_CLAMP:   return GL_CLAMP_TO_EDGE;    
    case TEXTURE_WRAP_BORDER:  return GL_CLAMP_TO_BORDER;
    default:                   return -1;
    }
}

void set_texture_wrap(s32 texture_index, Texture_Wrap_Type wrap_type) {
    auto &texture = render_registry.textures[texture_index];
    texture.wrap = wrap_type;
    
    const s32 type = gl_texture_type(texture.type);
    const s32 wrap = gl_texture_wrap(wrap_type);

    glBindTexture(type, texture.id);
    glTexParameteri(type, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(type, GL_TEXTURE_WRAP_T, wrap);
    glBindTexture(type, 0);
}

static s32 gl_texture_min_filter(Texture_Filter_Type filter, bool has_mipmaps) {
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

void set_texture_filter(s32 texture_index, Texture_Filter_Type filter_type) {
    auto &texture = render_registry.textures[texture_index];
    texture.filter = filter_type;

    const bool has_mipmaps = texture.flags & TEXTURE_FLAG_HAS_MIPMAPS;
    
    const s32 type = gl_texture_type(texture.type);
    const s32 min_filter = gl_texture_min_filter(filter_type, has_mipmaps);
    const s32 mag_filter = gl_texture_mag_filter(filter_type);

    glBindTexture(type, texture.id);
    glTexParameteri(type, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(type, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindTexture(type, 0);
}

void generate_texture_mipmaps(s32 texture_index) {
    auto &texture = render_registry.textures[texture_index];
    texture.flags |= TEXTURE_FLAG_HAS_MIPMAPS;

    const s32 type = gl_texture_type(texture.type);
    glBindTexture(type, texture.id);
    glGenerateMipmap(type);
    glBindTexture(type, 0);
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
	Font_Atlas *atlas = alloclt(Font_Atlas);
	atlas->font = font;
	atlas->texture_index = INVALID_INDEX;

	const u32 charcode_count = end_charcode - start_charcode + 1;
	atlas->metrics = allocltn(Font_Glyph_Metric, charcode_count);
	atlas->start_charcode = start_charcode;
	atlas->end_charcode = end_charcode;
	atlas->texture_index = create_texture(TEXTURE_TYPE_2D_ARRAY, TEXTURE_FORMAT_RGBA_8, font_size, font_size, null);

    set_texture_filter(atlas->texture_index, TEXTURE_FILTER_NEAREST);
    
	rescale_font_atlas(atlas, font_size);

	return atlas;
}

void rescale_font_atlas(Font_Atlas *atlas, s16 font_size) {
	if (atlas->texture_index == INVALID_INDEX) {
		error("Failed to rescale font atlas with invalid texture");
		return;
	}

	auto &texture = render_registry.textures[atlas->texture_index];
	Assert(texture.type == TEXTURE_TYPE_2D_ARRAY);

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
	Assert(charcode_count <= (u32)max_layers);

	// stbtt rasterizes glyphs as 8bpp, so tell GL to use 1 byte per color channel.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D_ARRAY, texture.id);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, atlas->font_size, atlas->font_size, charcode_count, 0, GL_RED, GL_UNSIGNED_BYTE, null);

	u8 *bitmap = (u8 *)allocl(font_size * font_size);
    defer { freel(font_size * font_size); };
        
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

		set_bytes(bitmap, 0, font_size * font_size);

		// @Cleanup: looks nasty, should come up with better solution.
		// Now we bake bitmap using stbtt and 'rescale' it to font size square one.
		// Maybe use stbtt_MakeGlyphBitmap at least?
		for (s32 y = 0; y < h; ++y) {
			for (s32 x = 0; x < w; ++x) {
				s32 src_index = y * w + x;
				s32 dest_index = (y + y_offset) * font_size + (x + x_offset);
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

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default color channel size
}
