#include "pch.h"
#include "render/render_init.h"
#include "render/glad.h"
#include "render/shader.h"
#include "render/uniform.h"
#include "render/material.h"
#include "render/texture.h"
#include "render/frame_buffer.h"
#include "render/buffer_storage.h"
#include "render/render_command.h"
#include "render/render_stats.h"
#include "render/mesh.h"
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

u32 R_USAGE_DRAW_STATIC  = GL_STATIC_DRAW;
u32 R_USAGE_DRAW_DYNAMIC = GL_DYNAMIC_DRAW;
u32 R_USAGE_DRAW_STREAM  = GL_STREAM_DRAW;

u32 R_FLAG_MAP_READ       = GL_MAP_READ_BIT;
u32 R_FLAG_MAP_WRITE      = GL_MAP_WRITE_BIT;
u32 R_FLAG_MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT;
u32 R_FLAG_MAP_COHERENT   = GL_MAP_COHERENT_BIT;

u32 R_FLAG_STORAGE_DYNAMIC = GL_DYNAMIC_STORAGE_BIT;
u32 R_FLAG_STORAGE_CLIENT  = GL_CLIENT_STORAGE_BIT;

void r_detect_capabilities() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &R_MAX_TEXTURE_TEXELS);
    
    R_UNIFORM_BUFFER_BASE_ALIGNMENT = 16;    
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS,     &R_MAX_UNIFORM_BUFFER_BINDINGS);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,          &R_MAX_UNIFORM_BLOCK_SIZE);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &R_MAX_VERTEX_ATTRIBUTES);
    for (s32 i = 0; i < R_MAX_VERTEX_ATTRIBUTES; ++i) {
        glEnableVertexAttribArray(i);
    }

    // MAX_VERTEX_ARRAY_BINDINGS index is used for entity id vertex array binding.
    Assert(MAX_VERTEX_ARRAY_BINDINGS + 1 <= R_MAX_VERTEX_ATTRIBUTES);
                    
    log("Render capabilities:");
    log("  Texture:\n    Max Size %dx%d texels", R_MAX_TEXTURE_TEXELS, R_MAX_TEXTURE_TEXELS);
    log("  Uniform buffer:\n    Max Bindings %d\n    Base Alignment %d\n    Offset Alignment %d",
        R_MAX_UNIFORM_BUFFER_BINDINGS, R_UNIFORM_BUFFER_BASE_ALIGNMENT, R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    log("  Uniform block:\n    Max Size %d bytes", R_MAX_UNIFORM_BLOCK_SIZE);
    log("  Vertex buffer:\n    Max Attributes %d", R_MAX_VERTEX_ATTRIBUTES);
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

void r_submit(const Render_Command *command) {    
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
    
    if (command->rid_frame_buffer != RID_NONE) {
        glBindFramebuffer(GL_FRAMEBUFFER, command->rid_frame_buffer);
    }

    if (command->sid_material != SID_NONE) {
        const auto &material = asset_table.materials[command->sid_material];
        if (material.sid_shader != SID_NONE) {
            const auto &shader = asset_table.shaders[material.sid_shader];

            glUseProgram(shader.rid);
            
            for (s32 i = 0; i < material.uniform_count; ++i) {
                r_set_uniform(shader.rid, material.uniforms + i);
            }
        }

        if (material.sid_texture != SID_NONE) {
            const auto &texture = asset_table.textures[material.sid_texture];
            glBindTextureUnit(0, texture.rid);
        }
    }

    if (command->rid_override_texture != SID_NONE) {
        glBindTextureUnit(0, command->rid_override_texture);
    }
    
    if (command->rid_vertex_array != SID_NONE) {
        draw_call_count += 1;

        glBindVertexArray(command->rid_vertex_array);

#if DEVELOPER
        // Add extra binding for entity id.
        glVertexArrayVertexBuffer(command->rid_vertex_array,
                                  EID_VERTEX_BINDING_INDEX,
                                  vertex_buffer_storage.rid,
                                  EID_VERTEX_DATA_OFFSET + command->eid_vertex_data_offset,
                                  sizeof(u32));
        glEnableVertexArrayAttrib(command->rid_vertex_array, EID_VERTEX_BINDING_INDEX);
        glVertexArrayAttribBinding(command->rid_vertex_array,
                                   EID_VERTEX_BINDING_INDEX, EID_VERTEX_BINDING_INDEX);
        glVertexArrayAttribIFormat(command->rid_vertex_array,
                                   EID_VERTEX_BINDING_INDEX, 1, GL_UNSIGNED_INT, 0);
        glVertexArrayBindingDivisor(command->rid_vertex_array,
                                    EID_VERTEX_BINDING_INDEX, 1);


#endif
        
        const s32 render_mode = gl_draw_mode(command->render_mode);
        if (command->flags & RENDER_FLAG_INDEXED) {
            // @Cleanup: this should be bound by default like vertex buffer storage.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_storage.rid);
            glDrawElementsInstancedBaseInstance(render_mode,
                                                command->buffer_element_count,
                                                GL_UNSIGNED_INT,
                                                (void *)(u64)command->buffer_element_offset,
                                                command->instance_count,
                                                command->instance_offset);
        } else {
            glDrawArraysInstancedBaseInstance(render_mode,
                                              command->buffer_element_offset,
                                              command->buffer_element_count,
                                              command->instance_count,
                                              command->instance_offset);
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

        if (command->rid_frame_buffer != RID_NONE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        
        if (command->rid_vertex_array != RID_NONE) {
            glBindVertexArray(0);
        }
        
        if (command->rid_override_texture != SID_NONE) {
            glBindTextureUnit(0, 0);
        }
        
        if (command->sid_material != SID_NONE) {
            const auto &material = asset_table.materials[command->sid_material];
            if (material.sid_shader != SID_NONE) {
                glUseProgram(0);
            }

            if (material.sid_texture != SID_NONE) {
                glBindTextureUnit(0, 0);
            }
        }
    }
}

void r_init_frame_buffer(s16 width, s16 height, const Texture_Format_Type *color_formats, s32 color_format_count, Texture_Format_Type depth_format, Frame_Buffer *frame_buffer) {
    Assert(color_format_count <= MAX_FRAME_BUFFER_COLOR_ATTACHMENTS);
    
    glGenFramebuffers(1, &frame_buffer->rid);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer->rid);

    for (s32 i = 0; i < MAX_FRAME_BUFFER_COLOR_ATTACHMENTS; ++i) {
        frame_buffer->color_attachments[i].rid_texture = RID_NONE;
        frame_buffer->color_attachments[i].format = color_formats[i];
    }
    
    u32 gl_color_attachments[MAX_FRAME_BUFFER_COLOR_ATTACHMENTS];
    for (s32 i = 0; i < color_format_count; ++i) {
        gl_color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    
    glDrawBuffers(color_format_count, gl_color_attachments);
    
    frame_buffer->color_attachment_count = color_format_count;

    frame_buffer->depth_attachment.rid_texture = RID_NONE;
    frame_buffer->depth_attachment.format = depth_format;

    frame_buffer->sid_material = SID("/data/materials/frame_buffer.mat");

    auto &material = asset_table.materials[frame_buffer->sid_material];
        
    const vec2 resolution = vec2(width, height);
    set_material_uniform_value(&material, "u_resolution", &resolution, sizeof(resolution));
    
    r_recreate_frame_buffer(frame_buffer, width, height);
}

void r_recreate_frame_buffer(Frame_Buffer *frame_buffer, s16 width, s16 height) {
    frame_buffer->width = width;
    frame_buffer->height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer->rid);	
    
    for (s32 i = 0; i < frame_buffer->color_attachment_count; ++i) {
        auto &attachment = frame_buffer->color_attachments[i];
        if (attachment.rid_texture != RID_NONE) {
            r_delete_texture(attachment.rid_texture);
            attachment.rid_texture = RID_NONE;
        }

        attachment.rid_texture = r_create_texture(TEXTURE_TYPE_2D, attachment.format, width, height, null);
        r_set_texture_filter(attachment.rid_texture, TEXTURE_FILTER_NEAREST, false);
        r_set_texture_wrap(attachment.rid_texture, TEXTURE_WRAP_BORDER);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, attachment.rid_texture, 0);        
    }

    {
        auto &attachment = frame_buffer->depth_attachment;
        if (attachment.rid_texture != RID_NONE) {
            r_delete_texture(attachment.rid_texture);
            attachment.rid_texture = RID_NONE;
        }

        attachment.rid_texture = r_create_texture(TEXTURE_TYPE_2D, attachment.format, width, height, null);
        r_set_texture_filter(attachment.rid_texture, TEXTURE_FILTER_NEAREST, false);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachment.rid_texture, 0);        
    }
        
    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        warn("Framebuffer %u is not complete, GL status 0x%X", frame_buffer->rid, status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

s32 r_read_frame_buffer_pixel(rid rid_frame_buffer, s32 color_attachment_index, s32 x, s32 y) {
    glBindFramebuffer(GL_FRAMEBUFFER, rid_frame_buffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    s32 pixel = -1;
    x = (s32)((f32)(x - viewport.x) * viewport.resolution_scale);
    y = (s32)((f32)(viewport.height - y - 1) * viewport.resolution_scale);
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return pixel;
}

rid rid_fb_va = RID_NONE;
u32 fb_index_data_offset = 0;
u32 fb_index_count = 0;

void r_init_frame_buffer_draw() {
    struct Vertex_Quad {
        vec2 pos;
        vec2 uv;
    };
    
    const Vertex_Quad vertices[] = {
        { vec2( 1.0f,  1.0f), vec2(1.0f, 1.0f) },
        { vec2( 1.0f, -1.0f), vec2(1.0f, 0.0f) },
        { vec2(-1.0f, -1.0f), vec2(0.0f, 0.0f) },
        { vec2(-1.0f,  1.0f), vec2(0.0f, 1.0f) },
    };

    Vertex_Array_Binding binding = {};
    binding.binding_index = 0;
    binding.data_offset = vertex_buffer_storage.size;
    binding.layout_size = 2;
    binding.layout[0] = { VERTEX_F32_2, 0 };
    binding.layout[1] = { VERTEX_F32_2, 0 };

    void *vertex_data = r_allocv(sizeof(vertices));
    copy_bytes(vertex_data, vertices, sizeof(vertices));
    
    rid_fb_va = r_create_vertex_array(&binding, 1);

    const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
    fb_index_count = COUNT(indices);
    fb_index_data_offset = index_buffer_storage.size;

    void *index_data = r_alloci(sizeof(indices));
    copy_bytes(index_data, indices, sizeof(indices));
}

void draw_frame_buffer(Frame_Buffer *frame_buffer, s32 color_attachment_index) {
    auto &material = asset_table.materials[frame_buffer->sid_material];
    
    set_material_uniform_value(&material, "u_pixel_size", &frame_buffer->pixel_size, sizeof(frame_buffer->pixel_size));
    set_material_uniform_value(&material, "u_curve_distortion_factor", &frame_buffer->curve_distortion_factor, sizeof(frame_buffer->curve_distortion_factor));
    set_material_uniform_value(&material, "u_chromatic_aberration_offset", &frame_buffer->chromatic_aberration_offset, sizeof(frame_buffer->chromatic_aberration_offset));
    set_material_uniform_value(&material, "u_quantize_color_count", &frame_buffer->quantize_color_count, sizeof(frame_buffer->quantize_color_count));
    set_material_uniform_value(&material, "u_noise_blend_factor", &frame_buffer->noise_blend_factor, sizeof(frame_buffer->noise_blend_factor));
    set_material_uniform_value(&material, "u_scanline_count", &frame_buffer->scanline_count, sizeof(frame_buffer->scanline_count));
    set_material_uniform_value(&material, "u_scanline_intensity", &frame_buffer->scanline_intensity, sizeof(frame_buffer->scanline_intensity));
    
    Render_Command command = {};
    command.flags = RENDER_FLAG_VIEWPORT | RENDER_FLAG_SCISSOR | RENDER_FLAG_CULL_FACE | RENDER_FLAG_INDEXED;
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

    command.rid_vertex_array = rid_fb_va;
    command.sid_material = frame_buffer->sid_material;
    command.rid_override_texture = frame_buffer->color_attachments[color_attachment_index].rid_texture;

    command.buffer_element_count = fb_index_count;
    command.buffer_element_offset = fb_index_data_offset;
    
    r_submit(&command);
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

rid r_create_storage(const void *data, u32 size, u32 flags) {
    rid rid;
	glCreateBuffers(1, &rid);
    glNamedBufferStorage(rid, size, data, flags);
    return rid;
}

rid r_create_buffer(const void *data, u32 size, u32 usage) {
    rid rid;
	glCreateBuffers(1, &rid);
	glNamedBufferData(rid, size, data, usage);
	return rid;
}

void *r_map_buffer(rid rid, u32 offset, u32 size, u32 flags) {
    return glMapNamedBufferRange(rid, offset, size, flags);
}

bool r_unmap_buffer(rid rid) {
    return glUnmapNamedBuffer(rid);
}

void r_flush_buffer(rid rid, u32 offset, u32 size) {
    glFlushMappedNamedBufferRange(rid, offset, size);
}

rid r_create_vertex_array(const Vertex_Array_Binding *bindings, s32 binding_count) {
    Assert(binding_count <= MAX_VERTEX_ARRAY_BINDINGS);
    
    rid rid = RID_NONE;
    glCreateVertexArrays(1, &rid);

    s32 attribute_index = 0;
    for (s32 binding_index = 0; binding_index < binding_count; ++binding_index) {        
        const auto &binding = bindings[binding_index];
        
        s32 vertex_size = 0;
        for (s32 j = 0; j < binding.layout_size; ++j) {
            vertex_size += get_vertex_component_size(binding.layout[j].type);
        }

        glVertexArrayVertexBuffer(rid, binding.binding_index, vertex_buffer_storage.rid,
                                  binding.data_offset, vertex_size);

        s32 offset = 0;
        for (s32 j = 0; j < binding.layout_size; ++j) {
            const auto &component = binding.layout[j];
            
            const s32 data_type = gl_vertex_data_type(component.type);
            const s32 dimension = get_vertex_component_dimension(component.type);

            glEnableVertexArrayAttrib(rid, attribute_index);
            glVertexArrayAttribBinding(rid, attribute_index, binding_index);
            
            if (component.type == VERTEX_S32 || component.type == VERTEX_U32) {
                glVertexArrayAttribIFormat(rid, attribute_index, dimension, data_type, offset);
            } else {
                glVertexArrayAttribFormat(rid, attribute_index, dimension, data_type, component.normalize, offset);
            }

            glVertexArrayBindingDivisor(rid, attribute_index, component.advance_rate);

            offset += get_vertex_component_size(component.type);
            attribute_index += 1;
        }
	}

    return rid;
}

static u32 gl_create_shader(GLenum type, const char *src) {
	const u32 shader = glCreateShader(type);
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
        *str_char_from_end(info_log, ASCII_NEW_LINE) = '\0';
        
		error("Failed to link shader program, gl reason %s", info_log);
		return INVALID_INDEX;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

void init_shader_asset(Shader *shader, void *data) {
    auto source = (const char *)data;
        
	char *vertex_src   = (char *)allocl(MAX_SHADER_SIZE);
	char *fragment_src = (char *)allocl(MAX_SHADER_SIZE);
    defer { freel(2 * MAX_SHADER_SIZE); };

	if (!parse_shader(source, vertex_src, fragment_src)) {
        error("Failed to parse shader %s", shader->path);
        return;
    }

	const u32 vertex_shader = gl_create_shader(GL_VERTEX_SHADER, vertex_src);
    if (vertex_shader == GL_INVALID_INDEX) {
        error("Failed to create vertex shader %s", shader->path);
        return;
    }
    
	const u32 fragment_shader = gl_create_shader(GL_FRAGMENT_SHADER, fragment_src);
    if (fragment_shader == GL_INVALID_INDEX) {
        error("Failed to create fragment shader %s", shader->path);
        return;
    }

	const rid rid_new_shader = (rid)gl_link_program(vertex_shader, fragment_shader);
    if (rid_new_shader > 0) {
        if (shader->rid != RID_NONE) {
            glDeleteProgram(shader->rid);
        }

        shader->rid = rid_new_shader;

        // Set or update uniform block bindings.
        Assert(COUNT(UNIFORM_BLOCK_NAMES) == COUNT(UNIFORM_BLOCK_BINDINGS));
        for (s32 i = 0; i < COUNT(UNIFORM_BLOCK_NAMES); ++i) {
            const u32 index = glGetUniformBlockIndex(shader->rid, UNIFORM_BLOCK_NAMES[i]);
            if (index != GL_INVALID_INDEX) {
                glUniformBlockBinding(shader->rid, index, UNIFORM_BLOCK_BINDINGS[i]);
            }   
        }
    } else {
        error("Failed to link shader %s", shader->path);
    }
}

const char *get_desired_shader_file_extension() {
    return ".glsl";
}

void r_set_uniform(rid rid_shader, const Uniform *uniform) {
    const auto &cache = uniform_value_cache;
    Assert(uniform->value_offset < cache.size);

    const s32 location = glGetUniformLocation(rid_shader, uniform->name);
    if (location < 0) {
        return;
    }
    
    const void *data = (u8 *)cache.data + uniform->value_offset;
    
    switch (uniform->type) {
	case UNIFORM_U32:     glUniform1uiv(location, uniform->count, (u32 *)data); break;
    case UNIFORM_F32:     glUniform1fv(location,  uniform->count, (f32 *)data); break;
    case UNIFORM_F32_2:   glUniform2fv(location,  uniform->count, (f32 *)data); break;
    case UNIFORM_F32_3:   glUniform3fv(location,  uniform->count, (f32 *)data); break;
    case UNIFORM_F32_4:   glUniform4fv(location,  uniform->count, (f32 *)data); break;
	case UNIFORM_F32_4X4: glUniformMatrix4fv(location, uniform->count, GL_FALSE, (f32 *)data); break;
	default:
		error("Failed to sync uniform %s of unknown type %d", uniform->name, uniform->type);
		break;
	}
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

rid r_create_uniform_buffer(u32 size) {
    rid rid = RID_NONE;
    glCreateBuffers(1, &rid);
    glNamedBufferData(rid, size, null, GL_STATIC_DRAW);
    return rid;
}

void r_add_uniform_block(rid rid_uniform_buffer, s32 shader_binding, const char *name, const Uniform_Block_Field *fields, s32 field_count, Uniform_Block *block) {
    const u32 size = get_uniform_block_size_gpu_aligned(fields, field_count);
    const u32 offset = UNIFORM_BUFFER_SIZE;

    UNIFORM_BUFFER_SIZE += size;
    UNIFORM_BUFFER_SIZE = Align(UNIFORM_BUFFER_SIZE, R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    Assert(UNIFORM_BUFFER_SIZE <= MAX_UNIFORM_BUFFER_SIZE);
    
    glBindBufferRange(GL_UNIFORM_BUFFER, shader_binding, rid_uniform_buffer, offset, size);

    block->rid_uniform_buffer = rid_uniform_buffer;
    block->name = name;
    copy_bytes(block->fields, fields, field_count * sizeof(Uniform_Block_Field));
    block->field_count = field_count;
    block->shader_binding = shader_binding;
    block->offset = offset;
    block->size = size;
}

void r_set_uniform_block_value(Uniform_Block *block, s32 field_index, s32 field_element_index, const void *data, u32 size) {
    const u32 field_offset = get_uniform_block_field_offset_gpu_aligned(block, field_index, field_element_index);
    r_set_uniform_block_value(block, field_offset, data, size);
}

void r_set_uniform_block_value(Uniform_Block *block, u32 offset, const void *data, u32 size) {
    Assert(offset + size <= block->size);
    glNamedBufferSubData(block->rid_uniform_buffer, block->offset + offset, size, data);
}

u32 get_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field) {
    if (field.count == 1) {
        return get_uniform_type_size_gpu_aligned(field.type);
    }

    return get_uniform_type_size_gpu_aligned(UNIFORM_F32_4) * field.count;
}

u32 get_uniform_block_field_offset_gpu_aligned(Uniform_Block *block, s32 field_index, s32 field_element_index) {
    Assert(field_index < block->field_count);
    Assert(field_element_index < block->fields[field_index].count);
    return get_uniform_block_field_offset_gpu_aligned(block->fields, field_index, field_element_index);
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

rid r_create_texture(Texture_Type type, Texture_Format_Type format, s32 width, s32 height, void *data) {
    const s32 gl_type            = gl_texture_type(type);
    const s32 gl_format          = gl_texture_format(format);
    const s32 gl_internal_format = gl_texture_internal_format(format);
    const s32 gl_data_type       = gl_texture_data_type(format);
    
    rid rid;
    glCreateTextures(gl_type, 1, &rid);
    r_set_texture_data(rid, type, format, width, height, data);

    return rid;
}

void r_set_texture_data(rid rid_texture, Texture_Type type, Texture_Format_Type format, s32 width, s32 height, void *data) {
    const s32 gl_type = gl_texture_type(type);
    const s32 gl_format = gl_texture_format(format);
    const s32 gl_internal_format = gl_texture_internal_format(format);
    const s32 gl_data_type = gl_texture_data_type(format);

    glBindTexture(gl_type, rid_texture);
    // @Todo: properly set data based on texture type.
    glTexImage2D(gl_type, 0, gl_internal_format, width, height, 0, gl_format, gl_data_type, data);
    glBindTexture(gl_type, 0);
}

static s32 gl_texture_wrap(Texture_Wrap_Type wrap) {
    switch (wrap) {
    case TEXTURE_WRAP_REPEAT:  return GL_REPEAT;
    case TEXTURE_WRAP_CLAMP:   return GL_CLAMP_TO_EDGE;    
    case TEXTURE_WRAP_BORDER:  return GL_CLAMP_TO_BORDER;
    default:                   return -1;
    }
}

void r_set_texture_wrap(rid rid_texture, Texture_Wrap_Type wrap) {
    const s32 gl_wrap = gl_texture_wrap(wrap);
    glTextureParameteri(rid_texture, GL_TEXTURE_WRAP_S, gl_wrap);
    glTextureParameteri(rid_texture, GL_TEXTURE_WRAP_T, gl_wrap);
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

void r_set_texture_filter(rid rid_texture, Texture_Filter_Type filter, bool has_mipmaps) {
    const s32 gl_min_filter = gl_texture_min_filter(filter, has_mipmaps);
    const s32 gl_mag_filter = gl_texture_mag_filter(filter);

    glTextureParameteri(rid_texture, GL_TEXTURE_MIN_FILTER, gl_min_filter);
    glTextureParameteri(rid_texture, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
}

void r_generate_texture_mipmaps(rid rid_texture) {
    glGenerateTextureMipmap(rid_texture);
}

void r_delete_texture(rid rid_texture) {
    glDeleteTextures(1, &rid_texture);
}

void rescale_font_atlas(Font_Atlas *atlas, s16 font_size) {
	if (atlas->rid_texture == RID_NONE) {
		error("Failed to rescale font atlas with invalid texture");
		return;
	}

	const Font_Info *font = atlas->font;
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

	glBindTexture(GL_TEXTURE_2D_ARRAY, atlas->rid_texture);
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
