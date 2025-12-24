#include "pch.h"
#include "glad.h"
#include "gpu.h"
#include "render.h"
#include "shader_binding_model.h"
#include "shader.h"
#include "material.h"
#include "texture.h"
#include "viewport.h"
#include "command_buffer.h"
#include "render_target.h"
#include "vertex_descriptor.h"
#include "file_system.h"
#include "font.h"
#include "profile.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_sprintf.h"

const u64 GPU_WAIT_INFINITE = GL_TIMEOUT_IGNORED;

static constexpr u32 GL_polygon_modes  [] = { GL_FILL, GL_LINE, GL_POINT };
static constexpr u32 GL_topology_modes [] = { GL_LINES, GL_TRIANGLES, GL_TRIANGLE_STRIP };
static constexpr u32 GL_cull_faces     [] = { GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
static constexpr u32 GL_windings       [] = { GL_CW, GL_CCW };
static constexpr u32 GL_blend_funcs    [] = { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
static constexpr u32 GL_depth_funcs    [] = { GL_LESS, GL_GREATER, GL_EQUAL, GL_NOTEQUAL, GL_LEQUAL, GL_GEQUAL };
static constexpr u32 GL_stencil_funcs  [] = { GL_ALWAYS, GL_KEEP, GL_REPLACE, GL_NOTEQUAL };
#define GL_stencil_ops GL_stencil_funcs

constexpr u32 GL_vertex_component_types[VC_COUNT] = {
    0,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
    GL_FLOAT,
};

void gpu_memory_barrier() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool post_init_render_backend() {
    auto buffer = get_gpu_buffer();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER,  buffer->handle._u32);

    // @Cleanup: make it less hardcoded
    gpu_picking_data = New(Gpu_Picking_Data, 1, gpu_allocator);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 8, buffer->handle._u32, get_gpu_buffer_offset(gpu_picking_data), sizeof(*gpu_picking_data));
    
    return true;
}

static u32 gl_clear_bits(u32 bits) {
    u32 out = 0;

    if (bits & CLEAR_COLOR_BIT)   out |= GL_COLOR_BUFFER_BIT;
    if (bits & CLEAR_DEPTH_BIT)   out |= GL_DEPTH_BUFFER_BIT;
    if (bits & CLEAR_STENCIL_BIT) out |= GL_STENCIL_BUFFER_BIT;
    
    return out;
}

void flush(Command_Buffer *buf) {
    static    u32 topology   = GL_TRIANGLES;
    constexpr u32 index_type = GL_UNSIGNED_INT;
    constexpr u32 index_size = sizeof(u32);
    
    for (u32 i = 0; i < buf->count; ++i) {
        const auto &cmd = buf->commands[i];
        switch (cmd.type) {
        case CMD_NONE: {
            log(LOG_MINIMAL, "Empty gpu command 0x%X in gpu command buffer 0x%X", &cmd, buf);
            break;
        }
        case CMD_POLYGON: {
            glPolygonMode(GL_FRONT_AND_BACK, GL_polygon_modes[cmd.polygon_mode]);
            break;
        }
        case CMD_TOPOLOGY: {
            topology = GL_topology_modes[cmd.topology_mode];
            break;
        }
        case CMD_VIEWPORT: {
            glViewport(cmd.x, cmd.y, cmd.w, cmd.h);
            break;
        }
        case CMD_SCISSOR: {
            glScissor(cmd.x, cmd.y, cmd.w, cmd.h);
            break;
        }
        case CMD_CULL: {
            glCullFace(GL_cull_faces[cmd.cull_face]);
            glFrontFace(GL_windings[cmd.cull_winding]);
            break;
        }
        case CMD_BLEND_WRITE: {
            if (cmd.blend_write) glEnable (GL_BLEND);
            else                 glDisable(GL_BLEND);
            
            break;
        }
        case CMD_BLEND_FUNC: {
            glBlendFunc(GL_blend_funcs[cmd.blend_src], GL_blend_funcs[cmd.blend_dst]);
            break;
        }
        case CMD_DEPTH_WRITE: {
            glDepthMask(cmd.depth_write);
            break;
        }
        case CMD_DEPTH_FUNC: {
            glDepthFunc(GL_depth_funcs[cmd.depth_func]);
            break;
        }
        case CMD_STENCIL_MASK: {
            glStencilMask(cmd.stencil_mask);
            break;
        };
        case CMD_STENCIL_FUNC: {
            glStencilFunc(GL_stencil_funcs[cmd.stencil_func],
                          cmd.stencil_func_cmp, cmd.stencil_func_mask);
            break;
        }
        case CMD_STENCIL_OP: {
            glStencilOp(GL_stencil_ops[cmd.stencil_op_fail],
                        GL_stencil_ops[cmd.stencil_op_depth_fail],
                        GL_stencil_ops[cmd.stencil_op_pass]);
            break;
        }
        case CMD_CLEAR: {
            glClearColor(cmd.clear_r, cmd.clear_g, cmd.clear_b, cmd.clear_a);
            glClear(gl_clear_bits(cmd.clear_bits));
            break;
        }
        case CMD_SHADER: {
            glUseProgram(cmd.shader->linked_program._u32);
            break;
        }
        case CMD_TEXTURE: {
            glBindTextureUnit(cmd.texture_binding, cmd.texture->handle._u32);
            break;
        }
        case CMD_VERTEX_DESCRIPTOR: {
            glBindVertexArray(cmd.vertex_descriptor->handle._u32);
            break;
        }
        case CMD_VERTEX_BINDING: {
            // @Cleanup: this whole feature is not finished, attribute_index stuff need to be
            // fixed for example.
            
            const auto handle        = cmd.vertex_descriptor_handle;
            const auto binding       = cmd.vertex_binding;
            const auto binding_index = binding->binding_index;

            s32 attribute_index = binding_index; // @Cleanup: we hope this is correct...
            u32 offset = 0;

            u32 vertex_size = 0;
            for (u32 c = 0; c < binding->component_count; ++c) {
                vertex_size += get_vertex_component_size(binding->components[c].type);
            }
            
            for (u32 i = 0; i < binding->component_count; ++i) {
                const auto &component = binding->components[i];
                const auto data_type  = GL_vertex_component_types[component.type];
                const auto dimension  = get_vertex_component_dimension(component.type);

                glVertexArrayVertexBuffer(handle._u32, attribute_index,
                                          get_gpu_buffer_handle()._u32,
                                          binding->offset, vertex_size);
                
                glEnableVertexArrayAttrib (handle._u32, attribute_index);
                glVertexArrayAttribBinding(handle._u32, attribute_index, binding_index);
            
                if (component.type == VC_S32 || component.type == VC_U32) {
                    glVertexArrayAttribIFormat(handle._u32, attribute_index, dimension, data_type, offset);
                } else {
                    glVertexArrayAttribFormat(handle._u32, attribute_index, dimension, data_type, component.normalize, offset);
                }

                glVertexArrayBindingDivisor(handle._u32, attribute_index, component.advance_rate);

                offset += get_vertex_component_size(component.type);
                attribute_index += 1;
            }
            
            break;
        }
        case CMD_RENDER_TARGET: {
            glBindFramebuffer(GL_FRAMEBUFFER, cmd.render_target->handle._u32);
            break;
        }
        case CMD_CBUFFER_INSTANCE: {            
            auto cbi = cmd.cbuffer_instance;
            auto cb  = cbi->constant_buffer;

            auto submit_buffer = get_submit_buffer();
            // Ensure next allocation for submit data is properly 
            // aligned with cbuffer offset requirements.
            align(submit_buffer, CBUFFER_OFFSET_ALIGNMENT);
            
            auto submit_data = alloc(submit_buffer, cb->size);
            For (cbi->value_table) {
                auto data = it.value.data;
                auto c    = it.value.constant;
                
                copy((u8 *)submit_data + c->offset, data, c->size);
                
                //
                // This is an interesting discover about difference between updating gpu
                // memory through mapped pointer or by using direct gl call. The former
                // updates data as is and memory is visible at gpu execution time, not at
                // submission time, so there was a bug that all entities used last set
                // entity parameters data. The latter copies data into drivers internal
                // staging area from which gpu will read. So in some cases like frequent
                // update of the same gpu memory region its better to use direct gl call
                // to ensure correct data will be used by gpu later.
                //
                //                                                   -alex, Nov 24 2025
                //
                // Turned out that the latter version (glNamedBufferSubData) was slow...
                //
            }

            auto handle = get_gpu_buffer_handle();
            auto offset = get_gpu_buffer_offset(submit_data);            
            glBindBufferRange(GL_UNIFORM_BUFFER, cb->binding, handle._u32, offset, cb->size);
            
            break;
        }
        case CMD_DRAW: {
            glDrawArraysInstancedBaseInstance(topology, cmd.first_vertex, cmd.vertex_count,
                                              cmd.instance_count, cmd.first_instance);
            inc_draw_call_count();
            break;
        }
        case CMD_DRAW_INDEXED: {
            glDrawElementsInstancedBaseInstance(topology, cmd.index_count, index_type,
                                                (const void *)(u64)(cmd.first_index * index_size),
                                                cmd.instance_count, cmd.first_instance);
            inc_draw_call_count();
            break;
        }
            
            // OpenGL docs state that the second parameter to indirect draw function
            // may be either offset to bound indirect buffer or direct pointer to memory
            // with indirect commands. Unfortunately, the latter didn't work, so the
            // only way is to use provided Gpu_Buffer which is sad, as it would be nice
            // to use pointer to Indirect_Draw_Command or Indexed_Indirect_Draw_Command.
            //
            //                                                   -alex, Oct 31 2025
            
        case CMD_DRAW_INDIRECT: {
            const auto offset = get_gpu_buffer_offset(cmd.indirect_buffer->data) + cmd.indirect_offset;
            glMultiDrawArraysIndirect(topology, (const void *)offset,
                                      cmd.indirect_count, cmd.indirect_stride);
            inc_draw_call_count();
            break;
        }
        case CMD_DRAW_INDEXED_INDIRECT: {
            const auto offset = get_gpu_buffer_offset(cmd.indirect_buffer->data) + cmd.indirect_offset;
            glMultiDrawElementsIndirect(topology, index_type, (const void *)offset,
                                        cmd.indirect_count, cmd.indirect_stride);
            inc_draw_call_count();
            break;
        }
        }
    }

    buf->count = 0;
}

static String generate_render_target_color_attachment_name() {
    static u32 counter = 0;
    auto name = sprint("%s_%u", "render_target_color_attachment", counter);
    counter += 1;
    return name;
}

static String generate_render_target_depth_attachment_name() {
    static u32 counter = 0;
    auto name = sprint("%s_%u", "render_target_depth_attachment", counter);
    counter += 1;
    return name;
}

Render_Target make_render_target(u16 w, u16 h, const Texture_Format_Type *color_formats, u32 color_format_count, Texture_Format_Type depth_format) {
    Render_Target target;
    target.width  = w;
    target.height = h;

    glGenFramebuffers(1, &target.handle._u32);
    glBindFramebuffer(GL_FRAMEBUFFER, target.handle._u32);

    u32 gl_color_attachments[MAX_COLOR_ATTACHMENTS];
    for (u8 i = 0; i < color_format_count; ++i) {
        Assert(target.color_attachment_count < color_format_count);
        auto &attachment = target.color_attachments[target.color_attachment_count];
        target.color_attachment_count += 1;
        
        Loaded_Texture color_texture;
        color_texture.path   = generate_render_target_color_attachment_name();
        color_texture.width  = w;
        color_texture.height = h;
        color_texture.type   = TEX_2D;
        color_texture.format = color_formats[i];
        
        attachment = new_texture(color_texture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, attachment->handle._u32, 0);
        
        gl_color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glDrawBuffers(color_format_count, gl_color_attachments);

    {
        auto &attachment = target.depth_attachment;
    
        Loaded_Texture depth_texture;
        depth_texture.path   = generate_render_target_depth_attachment_name();
        depth_texture.width  = w;
        depth_texture.height = h;
        depth_texture.type   = TEX_2D;
        depth_texture.format = depth_format;
        
        attachment = new_texture(depth_texture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D, attachment->handle._u32, 0);
    }

    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        log(LOG_VERBOSE, "Framebuffer %u is not complete, GL status 0x%X", target.handle._u32, status);
    }
    
    return target;
}

void resize(Render_Target &target, u16 w, u16 h) {
    target.width  = w;
    target.height = h;

    for (u8 i = 0; i < target.color_attachment_count; ++i) {
        auto &it = target.color_attachments[i];

        Loaded_Texture color_texture;
        color_texture.path   = it ? it->path : generate_render_target_color_attachment_name();
        color_texture.width  = w;
        color_texture.height = h;
        color_texture.type   = TEX_2D;
        color_texture.format = it ? it->format : TEX_RGB_8;
                
        it = new_texture(color_texture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, it->handle._u32, 0);
    }

    {
        auto &attachment = target.depth_attachment;
    
        Loaded_Texture depth_texture;
        depth_texture.path   = attachment ? attachment->path : generate_render_target_depth_attachment_name();
        depth_texture.width  = w;
        depth_texture.height = h;
        depth_texture.type   = TEX_2D;
        depth_texture.format = attachment ? attachment->format : TEX_DEPTH_24_STENCIL_8;
        
        attachment = new_texture(depth_texture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachment->handle._u32, 0);
    }

    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        log(LOG_VERBOSE, "Framebuffer %u is not complete, GL status 0x%X", target.handle._u32, status);
    }
}

u32 read_pixel(u32 color_attachment_index, u32 x, u32 y) {
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    u32 pixel = 0;
    glReadPixels(x, y, 1, 1, GL_RED, GL_UNSIGNED_INT, &pixel);
    
    return pixel;
}

Vertex_Descriptor make_vertex_descriptor(const Vertex_Binding *bindings, u32 binding_count) {
    Vertex_Descriptor vd;
    glCreateVertexArrays(1, &vd.handle._u32);

    s32 attribute_index = 0;
    for (u32 i = 0; i < binding_count; ++i) {        
        const auto &binding = bindings[i];
        const auto binding_index = binding.binding_index;
        
        Assert(vd.binding_count < binding_count);
        vd.bindings[vd.binding_count] = binding;
        vd.binding_count += 1;
        
        u32 vertex_size = 0;
        for (u32 c = 0; c < binding.component_count; ++c) {
            vertex_size += get_vertex_component_size(binding.components[c].type);
        }

        glVertexArrayVertexBuffer(vd.handle._u32, binding.binding_index, get_gpu_buffer_handle()._u32, binding.offset, vertex_size);

        u32 offset = 0;
        for (u32 j = 0; j < binding.component_count; ++j) {
            const auto &component = binding.components[j];
            const auto data_type = GL_vertex_component_types[component.type];
            const auto dimension = get_vertex_component_dimension(component.type);

            glEnableVertexArrayAttrib(vd.handle._u32, attribute_index);
            glVertexArrayAttribBinding(vd.handle._u32, attribute_index, binding_index);
            
            if (component.type == VC_S32 || component.type == VC_U32) {
                glVertexArrayAttribIFormat(vd.handle._u32, attribute_index, dimension, data_type, offset);
            } else {
                glVertexArrayAttribFormat(vd.handle._u32, attribute_index, dimension, data_type, component.normalize, offset);
            }

            glVertexArrayBindingDivisor(vd.handle._u32, attribute_index, component.advance_rate);

            offset += get_vertex_component_size(component.type);
            attribute_index += 1;
        }
	}

    glBindVertexArray(vd.handle._u32);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get_gpu_buffer_handle()._u32);
    glBindVertexArray(0);
    
    return vd;
}

static constexpr u32 GL_shader_types[SHADER_TYPE_COUNT] = {
    0, // none
    GL_VERTEX_SHADER,
    0, // hull
    0, // domain
    0, // geometry
    GL_FRAGMENT_SHADER,
    GL_COMPUTE_SHADER,
};

Shader *new_shader(const Compiled_Shader &compiled_shader) {        
    auto &platform = shader_platform;
    const auto &shader_file = *compiled_shader.shader_file;

    const u32 program = glCreateProgram();
    u32 *shaders = New(u32, shader_file.entry_point_count, __temporary_allocator);
    defer {
        for (u32 i = 0; i < shader_file.entry_point_count; ++i) {
            glDeleteShader(shaders[i]);
        }
    };

    for (u32 i = 0; i < shader_file.entry_point_count; ++i) {
        const auto &entry_point = shader_file.entry_points[i];
        const auto &source      = compiled_shader.compiled_sources[i];
        
        const u32 type   = GL_shader_types[entry_point.type];
        const u32 shader = glCreateShader(type);
        
        shaders[i] = shader;

        const auto src_data = (char *)source.data;
        const auto src_size = (GLint )source.size;
        
#if 1   // glsl
        glShaderSource(shader, 1, &src_data, &src_size);
        glCompileShader(shader);

        s32 success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            const char *shader_name = null;
            if (type == GL_VERTEX_SHADER)        shader_name = "vertex";
            else if (type == GL_FRAGMENT_SHADER) shader_name = "fragment";

            char info_log[1024];
            glGetShaderInfoLog(shader, sizeof(info_log), null, info_log);
            log(LOG_MINIMAL, "Failed to compile %s region from %S, gl reason %s", shader_name, shader_file.path, info_log);

            glDeleteProgram(program);
            return {};
        }
#else   // spirv
        const char *cname = str_c(*scratch.arena, entry_point.name);
            
        glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, src_data, src_size);
        glSpecializeShader(shader, cname, 0, null, null);

        // @Todo: check for success.
#endif

        glAttachShader(program, shader);
    }

    glLinkProgram(program);

	s32 success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
    
	if (!success) {
		char info_log[1024];
		glGetProgramInfoLog(program, sizeof(info_log), null, info_log);        
		log(LOG_MINIMAL, "Failed to link shader program for %S, gl reason %s", shader_file.path, info_log);
        glDeleteProgram(program);
		return {};
	}
    
    auto &shader = platform.shader_table[shader_file.name];
    shader.shader_file    = &shader_file;
    shader.linked_program._u32 = program;
    shader.resource_table.allocator = context.allocator;
    
    table_clear  (shader.resource_table);
    table_realloc(shader.resource_table, 8);

    For (compiled_shader.resources) {
        table_add(shader.resource_table, it.name, it);
    }
    
    return &shader;
}

static constexpr s32 gl_storage_bits(u32 bits) {
    u32 out = 0;

    if (bits & GPU_DYNAMIC_BIT)    out |= GL_DYNAMIC_STORAGE_BIT;
    if (bits & GPU_READ_BIT)       out |= GL_MAP_READ_BIT;
    if (bits & GPU_WRITE_BIT)      out |= GL_MAP_WRITE_BIT;
    if (bits & GPU_PERSISTENT_BIT) out |= GL_MAP_PERSISTENT_BIT;
    if (bits & GPU_COHERENT_BIT)   out |= GL_MAP_COHERENT_BIT;

    return out;
};

Gpu_Buffer make_gpu_buffer(u64 size, u32 bits) {
    Gpu_Buffer buffer;
    buffer.size = size;
    buffer.bits = bits;
    
    glCreateBuffers(1, &buffer.handle._u32);
    glNamedBufferStorage(buffer.handle._u32, size, null, gl_storage_bits(bits));
    
    return buffer;
}

void map(Gpu_Buffer *buffer, u64 size, u32 bits) {
    Assert(!buffer->mapped_pointer);
    Assert(size <= buffer->size);
    buffer->mapped_pointer = glMapNamedBufferRange(buffer->handle._u32, 0, size, gl_storage_bits(bits));
}

bool unmap(Gpu_Buffer *buffer) {
    Assert(buffer->mapped_pointer);
    buffer->mapped_pointer = null;
    return glUnmapNamedBuffer(buffer->handle._u32);
}

Gpu_Handle gpu_fence_sync() {
    Gpu_Handle sync = GPU_HANDLE_NONE;
    sync._p = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    return sync;
}

static Gpu_Sync_Result to_gpu_sync_result(u32 gl_wait_result) {
    switch (gl_wait_result) {
    case GL_ALREADY_SIGNALED:    return GPU_ALREADY_SIGNALED;
    case GL_TIMEOUT_EXPIRED:     return GPU_TIMEOUT_EXPIRED;
    case GL_CONDITION_SATISFIED: return GPU_SIGNALED;
    case GL_WAIT_FAILED:         return GPU_WAIT_FAILED;
    default: unreachable_code_path(); return GPU_WAIT_FAILED;
    }
}

Gpu_Sync_Result wait_client_sync(Gpu_Handle sync, u64 timeout) {
    auto result = glClientWaitSync((GLsync)sync._p, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    return to_gpu_sync_result(result);
}

void wait_gpu_sync(Gpu_Handle sync) {
    glWaitSync((GLsync)sync._p, 0, GL_TIMEOUT_IGNORED);
}

void delete_gpu_sync(Gpu_Handle sync) {
    glDeleteSync((GLsync)sync._p);
}

// @See: appropriate texture enums in texture.h

static constexpr s32 GL_texture_types[] = {
    0, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY,
};
static constexpr s32 GL_texture_formats[] = {
    0, GL_RED, GL_RED_INTEGER, GL_RED_INTEGER, GL_RGB, GL_RGBA, GL_DEPTH_STENCIL,
};
static constexpr s32 GL_texture_internal_formats[] = {
    0, GL_R8, GL_R32I, GL_R32UI, GL_RGB8, GL_RGBA8, GL_DEPTH24_STENCIL8,
};
static constexpr s32 GL_texture_data_types[] = {
    0, GL_UNSIGNED_BYTE, GL_INT, GL_UNSIGNED_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_24_8,
};
static constexpr s32 GL_unpack_alignments[] = {
    0, 1, 4, 4, 4, 4, 4
};
static constexpr s32 GL_texture_wraps[] = {
    0, GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER,
};
static constexpr s32 GL_texture_filters[] = {
    0, GL_LINEAR, GL_NEAREST,
    GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST,
    GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
};
static constexpr s32 GL_default_unpack_alignment = 4;

Texture *new_texture(const Loaded_Texture &loaded_texture) {
    auto path = copy_string(loaded_texture.path);
    auto name = get_file_name_no_ext(path);
    
    const s32 type            = GL_texture_types[loaded_texture.type];
    const s32 level           = 0;
    const s32 format          = GL_texture_formats[loaded_texture.format];
    const s32 internal_format = GL_texture_internal_formats[loaded_texture.format];
    const s32 border          = 0;
    const s32 data_type       = GL_texture_data_types[loaded_texture.format];

    Gpu_Handle handle = GPU_HANDLE_NONE;
    glCreateTextures(type, 1, &handle._u32);

    if (handle == GPU_HANDLE_NONE) {
        log(LOG_MINIMAL, "Failed to create GL texture");
        return global_textures.error;
    }

    const s32 unpack_alignment = GL_unpack_alignments[loaded_texture.format];
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    
    const auto &contents = loaded_texture.contents;
    glBindTexture(type, handle._u32);
    // @Todo: properly set data based on texture type.
    glTexImage2D(type, level, internal_format, loaded_texture.width,
                 loaded_texture.height, border, format, data_type, contents.data);
    glBindTexture(type, 0);

    auto &texture = texture_table[name];
    if (texture.handle != GPU_HANDLE_NONE) {
        glDeleteTextures(1, &texture.handle._u32);
    }

    // @Todo: set texture address.

    texture.path   = path;
    texture.name   = name;
    texture.handle = handle;
    texture.width  = loaded_texture.width;
    texture.height = loaded_texture.height;
    texture.type   = loaded_texture.type;
    texture.format = loaded_texture.format;

    texture.wrap       = TEX_REPEAT;
    texture.min_filter = TEX_NEAREST_MIPMAP_LINEAR;
    texture.mag_filter = TEX_NEAREST;

    const s32 wrap       = GL_texture_wraps  [texture.wrap];
    const s32 min_filter = GL_texture_filters[texture.min_filter];
    const s32 mag_filter = GL_texture_filters[texture.mag_filter];

    glTextureParameteri(handle._u32, GL_TEXTURE_WRAP_S, wrap);
    glTextureParameteri(handle._u32, GL_TEXTURE_WRAP_T, wrap);

    glTextureParameteri(handle._u32, GL_TEXTURE_MIN_FILTER, min_filter);
    glTextureParameteri(handle._u32, GL_TEXTURE_MAG_FILTER, mag_filter);

    glGenerateTextureMipmap(handle._u32);

    glPixelStorei(GL_UNPACK_ALIGNMENT, GL_default_unpack_alignment);

    return &texture;
}

void update_texture_filters(Texture &texture, Texture_Filter_Type min_filter, Texture_Filter_Type mag_filter) {
    texture.min_filter = min_filter;
    texture.mag_filter = mag_filter;

    const s32 gl_min_filter = GL_texture_filters[texture.min_filter];
    const s32 gl_mag_filter = GL_texture_filters[texture.mag_filter];
    
    glTextureParameteri(texture.handle._u32, GL_TEXTURE_MIN_FILTER, gl_min_filter);
    glTextureParameteri(texture.handle._u32, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
}
