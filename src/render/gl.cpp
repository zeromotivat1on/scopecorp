#include "pch.h"
#include "glad.h"
#include "gpu.h"
#include "render.h"
#include "shader_binding_model.h"
#include "shader.h"
#include "material.h"
#include "texture.h"
#include "viewport.h"
#include "file_system.h"
#include "font.h"
#include "profile.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_sprintf.h"

const u64 GPU_WAIT_INFINITE = GL_TIMEOUT_IGNORED;

static inline u32 to_gl_polygon_mode(Gpu_Polygon_Mode mode) {
    constexpr u32 lut[] = { GL_NONE, GL_FILL, GL_LINE, GL_POINT };
    return lut[mode];
}

static inline u32 to_gl_topology_mode(Gpu_Topology_Mode mode) {
    constexpr u32 lut[] = { GL_NONE, GL_LINES, GL_TRIANGLES, GL_TRIANGLE_STRIP };
    return lut[mode];
}

static inline u32 to_gl_cull_face(Gpu_Cull_Face face) {
    constexpr u32 lut[] = { GL_NONE, GL_FRONT, GL_BACK, GL_FRONT_AND_BACK };
    return lut[face];
}

static inline u32 to_gl_winding(Gpu_Winding_Type winding) {
    constexpr u32 lut[] = { GL_NONE, GL_CW, GL_CCW };
    return lut[winding];
}

static inline u32 to_gl_blend_func(Gpu_Blend_Function func) {
    constexpr u32 lut[] = { GL_NONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
    return lut[func];
}

static inline u32 to_gl_depth_func(Gpu_Depth_Function func) {
    constexpr u32 lut[] = {
        GL_NONE, GL_LESS, GL_GREATER,
        GL_EQUAL, GL_NOTEQUAL, GL_LEQUAL, GL_GEQUAL
    };
    return lut[func];
}

static inline u32 to_gl_stencil_func(Gpu_Stencil_Function func) {
    constexpr u32 lut[] = { GL_NONE, GL_ALWAYS, GL_KEEP, GL_REPLACE, GL_NOTEQUAL };
    return lut[func];
}

void gpu_memory_barrier() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool post_init_render_backend() {
    auto read_buffer  = gpu_get_buffer(gpu_read_allocator.buffer);
    auto write_buffer = gpu_get_buffer(gpu_write_allocator.buffer);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, write_buffer->handle._u32);

    // @Cleanup: make it less hardcoded
    auto gpu_allocation = gpu_alloc(sizeof(Gpu_Picking_Data), &gpu_read_allocator);
    gpu_picking_data = (Gpu_Picking_Data *)gpu_allocation.mapped_data;
    gpu_picking_data_offset = gpu_allocation.offset;
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 8, read_buffer->handle._u32, gpu_picking_data_offset, sizeof(*gpu_picking_data));
    
    return true;
}

static u32 to_gl_clear_bits(u32 bits) {
    u32 out = 0;

    if (bits & GPU_CLEAR_COLOR_BIT)   out |= GL_COLOR_BUFFER_BIT;
    if (bits & GPU_CLEAR_DEPTH_BIT)   out |= GL_DEPTH_BUFFER_BIT;
    if (bits & GPU_CLEAR_STENCIL_BIT) out |= GL_STENCIL_BUFFER_BIT;
    
    return out;
}

void gpu_flush_cmd_buffer(u32 cmd_buffer) {
    constexpr u32 index_type = GL_UNSIGNED_INT;
    constexpr u32 index_size = sizeof(u32);

    auto gpu_cmd_buffer = gpu_get_command_buffer(cmd_buffer);
    
    for (u32 i = 0; i < gpu_cmd_buffer->count; ++i) {
        const auto &cmd = gpu_cmd_buffer->commands[i];
        switch (cmd.type) {
        case GPU_CMD_NONE: {
            log(LOG_IDENT_GL, LOG_ERROR, "Empty gpu command 0x%X in gpu command buffer 0x%X", &cmd, gpu_cmd_buffer);
            break;
        }
        case GPU_CMD_POLYGON: {
            const auto gl_mode = to_gl_polygon_mode(cmd.polygon);
            glPolygonMode(GL_FRONT_AND_BACK, gl_mode);
            break;
        }
        case GPU_CMD_VIEWPORT: {
            glViewport(cmd.x, cmd.y, cmd.w, cmd.h);
            break;
        }
        case GPU_CMD_SCISSOR: {
            glScissor(cmd.x, cmd.y, cmd.w, cmd.h);
            break;
        }
        case GPU_CMD_CULL_FACE: {
            const auto gl_face = to_gl_cull_face(cmd.cull_face);
            glCullFace(gl_face);
            break;
        }
        case GPU_CMD_WINDING: {
            const auto gl_winding = to_gl_winding(cmd.winding);
            glFrontFace(gl_winding);
            break;
        }
        case GPU_CMD_SCISSOR_TEST: {
            if (cmd.scissor_test) glEnable (GL_SCISSOR_TEST);
            else                  glDisable(GL_SCISSOR_TEST);
            
            break;
        }
        case GPU_CMD_BLEND_TEST: {
            if (cmd.blend_test) glEnable (GL_BLEND);
            else                glDisable(GL_BLEND);
            
            break;
        }
        case GPU_CMD_BLEND_FUNC: {
            const auto gl_src = to_gl_blend_func(cmd.blend_src);
            const auto gl_dst = to_gl_blend_func(cmd.blend_dst);
            glBlendFunc(gl_src, gl_dst);
            break;
        }
        case GPU_CMD_DEPTH_TEST: {
            if (cmd.depth_test) glEnable (GL_DEPTH_TEST);
            else                glDisable(GL_DEPTH_TEST);
            break;
        }
        case GPU_CMD_DEPTH_WRITE: {
            glDepthMask(cmd.depth_write);
            break;
        }
        case GPU_CMD_DEPTH_FUNC: {
            const auto gl_func = to_gl_depth_func(cmd.depth_func);
            glDepthFunc(gl_func);
            break;
        }
        case GPU_CMD_STENCIL_MASK: {
            glStencilMask(cmd.stencil_global_mask);
            break;
        };
        case GPU_CMD_STENCIL_FUNC: {
            const auto gl_func = to_gl_stencil_func(cmd.stencil_test);
            glStencilFunc(gl_func, cmd.stencil_ref, cmd.stencil_mask);
            break;
        }
        case GPU_CMD_STENCIL_OP: {
            const auto gl_fail       = to_gl_stencil_func(cmd.stencil_fail);
            const auto gl_pass       = to_gl_stencil_func(cmd.stencil_pass);
            const auto gl_depth_fail = to_gl_stencil_func(cmd.stencil_depth_fail);
            glStencilOp(gl_fail, gl_depth_fail, gl_pass);
            break;
        }
        case GPU_CMD_CLEAR: {
            glClearColor(cmd.clear_color.r, cmd.clear_color.g, cmd.clear_color.b, cmd.clear_color.a);
            glClear(to_gl_clear_bits(cmd.clear_bits));
            break;
        }
        case GPU_CMD_SHADER: {
            // @Todo
            //const auto shader = gpu_get_shader(cmd.bind_resource);
            const auto shader = cmd.resource_handle;
            glUseProgram(shader._u32);
            break;
        }
        case GPU_CMD_IMAGE_VIEW: {
            const auto image_view = gpu_get_image_view(cmd.bind_resource);
            glBindTextureUnit(cmd.bind_index, image_view->handle._u32);
            break;
        }
        case GPU_CMD_SAMPLER: {
            const auto sampler = gpu_get_sampler(cmd.bind_resource);
            glBindSampler(cmd.bind_index, sampler->handle._u32);
            break;
        }
        // case GPU_CMD_TEXTURE: {
        //     glBindTextureUnit(cmd.texture_binding, cmd.texture->handle._u32);
        //     break;
        // }
        case GPU_CMD_VERTEX_INPUT: {
            const auto vertex_input = gpu_get_vertex_input(cmd.bind_resource);
            glBindVertexArray(vertex_input->handle._u32);
            break;
        }
        case GPU_CMD_VERTEX_BINDING: {
            // @Cleanup: this whole feature is not finished, attribute_index stuff need to be
            // fixed for example.
            
            // const auto handle        = it.vertex_descriptor_handle;
            // const auto binding       = it.vertex_binding;
            // const auto binding_index = binding->binding_index;

            // const auto write_buffer = gpu_get_buffer(gpu_write_allocator.buffer);
            
            // s32 attribute_index = binding_index; // @Cleanup: we hope this is correct...
            // u32 offset = 0;

            // u32 vertex_size = 0;
            // for (u32 c = 0; c < binding->component_count; ++c) {
            //     vertex_size += get_vertex_component_size(binding->components[c].type);
            // }
            
            // for (u32 i = 0; i < binding->component_count; ++i) {
            //     const auto &component = binding->components[i];
            //     const auto data_type  = GL_vertex_component_types[component.type];
            //     const auto dimension  = get_vertex_component_dimension(component.type);

            //     glVertexArrayVertexBuffer(handle._u32, attribute_index,
            //                               write_buffer->handle._u32,
            //                               binding->offset, vertex_size);
                
            //     glEnableVertexArrayAttrib (handle._u32, attribute_index);
            //     glVertexArrayAttribBinding(handle._u32, attribute_index, binding_index);
            
            //     if (component.type == VC_S32 || component.type == VC_U32) {
            //         glVertexArrayAttribIFormat(handle._u32, attribute_index, dimension, data_type, offset);
            //     } else {
            //         glVertexArrayAttribFormat(handle._u32, attribute_index, dimension, data_type, component.normalize, offset);
            //     }

            //     glVertexArrayBindingDivisor(handle._u32, attribute_index, component.advance_rate);

            //     offset += get_vertex_component_size(component.type);
            //     attribute_index += 1;
            // }
            
            break;
        }
        case GPU_CMD_VERTEX_BUFFER: {
            const auto buffer = gpu_get_buffer(cmd.bind_resource);
            glBindVertexBuffer(cmd.bind_index, buffer->handle._u32, cmd.bind_offset, cmd.bind_stride);
            break;
        }
        case GPU_CMD_INDEX_BUFFER: {
            const auto buffer = gpu_get_buffer(cmd.bind_resource);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->handle._u32);
            break;
        }
        case GPU_CMD_FRAMEBUFFER: {
            const auto framebuffer = gpu_get_framebuffer(cmd.bind_resource);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->handle._u32);
            break;
        }
        case GPU_CMD_CBUFFER_INSTANCE: {
            const auto buffer = gpu_get_buffer(cmd.bind_resource);
            glBindBufferRange(GL_UNIFORM_BUFFER, cmd.bind_index, buffer->handle._u32, cmd.bind_offset, cmd.bind_size);
            
            break;
        }
        case GPU_CMD_DRAW: {
            const auto gl_topology = to_gl_topology_mode(cmd.topology);
            glDrawArraysInstancedBaseInstance(gl_topology, cmd.first_draw, cmd.draw_count,
                                              cmd.instance_count, cmd.first_instance);
            inc_draw_call_count();
            break;
        }
        case GPU_CMD_DRAW_INDEXED: {
            const auto gl_topology = to_gl_topology_mode(cmd.topology);
            glDrawElementsInstancedBaseInstance(gl_topology, cmd.draw_count, index_type,
                                                (const void *)(u64)(cmd.first_draw * index_size),
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
            
        case GPU_CMD_DRAW_INDIRECT: {
            const auto gl_topology = to_gl_topology_mode(cmd.topology);
            const auto offset = cmd.indirect_offset;
            glMultiDrawArraysIndirect(gl_topology, (const void *)offset, cmd.indirect_count, cmd.indirect_stride);
            inc_draw_call_count();
            break;
        }
        case GPU_CMD_DRAW_INDEXED_INDIRECT: {
            const auto gl_topology = to_gl_topology_mode(cmd.topology);
            const u64 offset = cmd.indirect_offset;
            glMultiDrawElementsIndirect(gl_topology, index_type, (const void *)offset, cmd.indirect_count, cmd.indirect_stride);
            inc_draw_call_count();
            break;
        }
        }
    }

    gpu_cmd_buffer->count = 0;
}

u32 read_pixel(u32 color_attachment_index, u32 x, u32 y) {
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    u32 pixel = 0;
    glReadPixels(x, y, 1, 1, GL_RED, GL_UNSIGNED_INT, &pixel);
    
    return pixel;
}

static inline u32 to_gl_vertex_data_type(Gpu_Vertex_Attribute_Type type) {
    constexpr u32 lut[] = {
        0, GL_INT, GL_UNSIGNED_INT,
        GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT,        
    };
    return lut[type];
}

static inline u32 to_gl_advance_rate(Gpu_Vertex_Input_Rate rate) {
    constexpr u32 lut[] = { 0, 0, 1 };
    return lut[rate];
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
            log(LOG_IDENT_GL, LOG_ERROR, "Failed to compile %s shader %S", shader_name, shader_file.path);
            if (info_log[0]) log(LOG_IDENT_GL, LOG_ERROR, "%s", info_log);
        }

        glAttachShader(program, shader);
    }

    glLinkProgram(program);

	s32 success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
    
	if (!success) {
		char info_log[1024];
		glGetProgramInfoLog(program, sizeof(info_log), null, info_log);        
		log(LOG_IDENT_GL, LOG_ERROR, "Failed to link shader %S", shader_file.path);
        if (info_log[0]) log(LOG_IDENT_GL, LOG_ERROR, "%s", info_log);
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

void gpu_init_backend() {
    gpu.vendor                  = make_string((u8 *)glGetString(GL_VENDOR));
    gpu.renderer                = make_string((u8 *)glGetString(GL_RENDERER));
    gpu.backend_version         = make_string((u8 *)glGetString(GL_VERSION));
    gpu.shader_language_version = make_string((u8 *)glGetString(GL_SHADING_LANGUAGE_VERSION));
}

u32 gpu_uniform_buffer_max_size         () { s32 v = 0; glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &v); return v; }
u32 gpu_uniform_buffer_offset_alignment () { s32 v = 0; glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &v); return v; }
u32 gpu_image_max_size                  () { s32 v = 0; glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v); return v; }
u32 gpu_vertex_attribute_max_count      () { s32 v = 0; glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &v); return v; }

static inline u32 to_gl_map_bits(Gpu_Buffer_Type type) {
    switch (type) {
    case GPU_BUFFER_TYPE_STAGING_CACHED:   return GL_MAP_READ_BIT  | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    case GPU_BUFFER_TYPE_STAGING_UNCACHED: return GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    default:                               return 0;
    }
}

u32 gpu_new_buffer(Gpu_Buffer_Type type, u64 size) {
    const auto gl_map_bits = to_gl_map_bits(type);
    
    const auto index = gpu_next_index(gpu.buffers, gpu.free_buffers);

    auto &buffer = gpu.buffers[index];
    auto &gl_id  = buffer.handle._u32; Assert(!gl_id);

    glCreateBuffers(1, &gl_id);
    glNamedBufferStorage(gl_id, size, null, gl_map_bits);

    buffer.type        = type;
    buffer.size        = size;
    buffer.mapped_data = glMapNamedBufferRange(gl_id, 0, size, gl_map_bits);
    
    return index;
}

static inline u32 to_gl_texture_type(Gpu_Image_Type type) {
    constexpr u32 lut[] = {
        GL_NONE, GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY,
    };

    return lut[type];
}

static inline u32 to_gl_internal_format(Gpu_Image_Format format) {
    constexpr u32 lut[] = {
        GL_NONE, GL_R8, GL_R32I, GL_R32UI, GL_RGB8, GL_RGBA8, GL_DEPTH24_STENCIL8,
    };

    return lut[format];
}

static inline u32 to_gl_format(Gpu_Image_Format format) {
    constexpr u32 lut[] = {
        GL_NONE, GL_RED, GL_RED_INTEGER, GL_RED_INTEGER, GL_RGB, GL_RGBA, GL_DEPTH_STENCIL,
    };

    return lut[format];
}

static inline u32 to_gl_data_type(Gpu_Image_Format format) {
    constexpr u32 lut[] = {
        GL_NONE, GL_UNSIGNED_BYTE, GL_INT, GL_UNSIGNED_INT,
        GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_24_8,
    };

    return lut[format];
}

static inline u32 to_gl_unpack_alignment(Gpu_Image_Format format) {
    constexpr u32 lut[] = { 0, 1, 4, 4, 4, 4, 4, };
    return lut[format];
}

u32 gpu_new_image(Gpu_Image_Type type, Gpu_Image_Format format, u32 mipmap_count, u32 width, u32 height, u32 depth, Buffer base_mipmap_contents) {
    Assert(mipmap_count);
    Assert(mipmap_count <= gpu_max_mipmap_count(width, height));

    const auto gl_type            = to_gl_texture_type(type);
    const auto gl_internal_format = to_gl_internal_format(format);
    const auto gl_format          = to_gl_format(format);
    const auto gl_data_type       = to_gl_data_type(format);

    switch (gl_format) {
    case GL_R32I:
    case GL_R32UI:
    case GL_DEPTH24_STENCIL8:
        mipmap_count = 1;
        break;
    };

    const auto index = gpu_next_index(gpu.images, gpu.free_images);
    
    auto &image = gpu.images[index];
    auto &gl_id = image.handle._u32; Assert(!gl_id);

    image.type         = type;
    image.format       = format;
    image.mipmap_count = mipmap_count;
    image.width        = width;
    image.height       = height;
    image.depth        = depth;

    glPixelStorei(GL_UNPACK_ALIGNMENT, to_gl_unpack_alignment(format));
    
    glCreateTextures(gl_type, 1, &gl_id);
    
    switch (gl_type) {
    case GL_TEXTURE_1D:
        glTextureStorage1D(gl_id, mipmap_count, gl_internal_format, width);
        if (base_mipmap_contents) {
            glTextureSubImage1D(gl_id, 0, 0, width, gl_format, gl_data_type, base_mipmap_contents.data);
        }
        break;

    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D:
        glTextureStorage2D(gl_id, mipmap_count, gl_internal_format, width, height);
        if (base_mipmap_contents) {
            glTextureSubImage2D(gl_id, 0, 0, 0, width, height, gl_format, gl_data_type, base_mipmap_contents.data);
        }
        break;

    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_3D:
    case GL_TEXTURE_CUBE_MAP:
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        glTextureStorage3D(gl_id, mipmap_count, gl_internal_format, width, height, depth);
        if (base_mipmap_contents) {
            glTextureSubImage3D(gl_id, 0, 0, 0, 0, width, height, depth, gl_format, gl_data_type, base_mipmap_contents.data);
        }
        break;

    default:
        log(LOG_IDENT_GL, LOG_ERROR, "Unknown/unsupported texture type %u", gl_type);
        break;
    }

    glGenerateTextureMipmap(gl_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    return index;
}

u32 gpu_new_image_view(u32 image, Gpu_Image_Type type, Gpu_Image_Format format, u32 mipmap_min, u32 mipmap_count, u32 depth_min, u32 depth_size) {
    const auto index = gpu_next_index(gpu.image_views, gpu.free_image_views);
    
    auto &image_view = gpu.image_views[index];
    auto &gl_id      = image_view.handle._u32; Assert(!gl_id);
    
    image_view.image        = image;
    image_view.type         = type;
    image_view.format       = format;
    image_view.mipmap_min   = mipmap_min;
    image_view.mipmap_count = mipmap_count;
    image_view.depth_min    = depth_min;
    image_view.depth_size   = depth_size;
    
    const auto &gpu_image     = gpu.images[image];
    const auto gl_original_id = gpu_image.handle._u32;

    const auto gl_type            = to_gl_texture_type(type);
    const auto gl_internal_format = to_gl_internal_format(format);

    glGenTextures(1, &gl_id);    
    glTextureView(gl_id, gl_type, gl_original_id, gl_internal_format, mipmap_min, mipmap_count, depth_min, depth_size);

    return index;
}

static inline u32 to_gl_filter(Gpu_Sampler_Filter filter) {
    constexpr u32 lut[] = {
        GL_NONE, GL_NEAREST, GL_LINEAR,
        GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
        GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR,
    };

    return lut[filter];
}

static inline u32 to_gl_wrap(Gpu_Sampler_Wrap wrap) {
    constexpr u32 lut[] = {
        GL_NONE, GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER,
        GL_MIRRORED_REPEAT, GL_MIRROR_CLAMP_TO_EDGE,
    };

    return lut[wrap];
}

static inline u32 to_gl_compare_mode(Gpu_Sampler_Compare_Mode mode) {
    constexpr u32 lut[] = {
        GL_NONE, GL_COMPARE_REF_TO_TEXTURE,
    };

    return lut[mode];
}

static inline u32 to_gl_compare_function(Gpu_Sampler_Compare_Function function) {
    constexpr u32 lut[] = {
        GL_NONE, GL_EQUAL, GL_NOTEQUAL, GL_LESS, GL_GREATER,
        GL_LEQUAL, GL_GEQUAL, GL_ALWAYS, GL_NEVER,
    };

    return lut[function];
}

u32 gpu_new_sampler(Gpu_Sampler_Filter filter_min, Gpu_Sampler_Filter filter_mag,
                    Gpu_Sampler_Wrap wrap_u, Gpu_Sampler_Wrap wrap_v, Gpu_Sampler_Wrap wrap_w,
                    Gpu_Sampler_Compare_Mode compare_mode, Gpu_Sampler_Compare_Function compare_function,
                    f32 lod_min, f32 lod_max, Color4f color_border) {
    const auto index = gpu_next_index(gpu.samplers, gpu.free_samplers);

    auto &sampler = gpu.samplers[index];
    auto &gl_id   = sampler.handle._u32; Assert(!gl_id);

    glCreateSamplers(1, &gl_id);

    const auto gl_filter_min = to_gl_filter(filter_min);
    const auto gl_filter_mag = to_gl_filter(filter_mag);
    const auto gl_wrap_u     = to_gl_wrap(wrap_u);
    const auto gl_wrap_v     = to_gl_wrap(wrap_v);
    const auto gl_wrap_w     = to_gl_wrap(wrap_w);
    const auto gl_cmp_mode   = to_gl_compare_mode(compare_mode);
    const auto gl_cmp_func   = to_gl_compare_function(compare_function);
    
    if (gl_filter_min) glSamplerParameteri(gl_id, GL_TEXTURE_MIN_FILTER,   gl_filter_min);
    if (gl_filter_mag) glSamplerParameteri(gl_id, GL_TEXTURE_MAG_FILTER,   gl_filter_mag);
    if (gl_wrap_u)     glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_S,       gl_wrap_u);
    if (gl_wrap_v)     glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_T,       gl_wrap_v);
    if (gl_wrap_w)     glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_R,       gl_wrap_w);
    if (gl_cmp_mode)   glSamplerParameteri(gl_id, GL_TEXTURE_COMPARE_MODE, gl_cmp_mode);
    if (gl_cmp_func)   glSamplerParameteri(gl_id, GL_TEXTURE_COMPARE_FUNC, gl_cmp_func);
    if (lod_min)       glSamplerParameterf(gl_id, GL_TEXTURE_MIN_LOD,      lod_min);
    if (lod_max)       glSamplerParameterf(gl_id, GL_TEXTURE_MAX_LOD,      lod_max);

    glSamplerParameterfv(gl_id, GL_TEXTURE_BORDER_COLOR, &color_border.r);

    sampler.filter_min       = filter_min;
    sampler.filter_mag       = filter_mag;
    sampler.wrap_u           = wrap_u;
    sampler.wrap_v           = wrap_v;
    sampler.wrap_w           = wrap_w;
    sampler.compare_mode     = compare_mode;
    sampler.compare_function = compare_function;
    sampler.lod_min          = lod_min;
    sampler.lod_max          = lod_max;
    sampler.color_border     = color_border;
    
    return index;
}

static inline u32 to_gl_shader_type(Gpu_Shader_Stage_Type stage) {
    constexpr u32 lut[] = {
        GL_NONE, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER,
        GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER,
        GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER,
    };
    return lut[stage];
}

u32 gpu_new_shader(Gpu_Shader_Stage_Type stage, Buffer source) {
    const auto index = gpu_next_index(gpu.shaders, gpu.free_shaders);

    auto &shader = gpu.shaders[index];
    auto &gl_id  = shader.handle._u32; Assert(!gl_id);

    const auto gl_type = to_gl_shader_type(stage);
    gl_id = glCreateShader(gl_type);

    const auto size = (s32)source.size;
    const auto data = (const char *)source.data;
    glShaderSource(gl_id, 1, &data, &size);
    glCompileShader(gl_id);

    s32 success;
    glGetShaderiv(gl_id, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        const auto stage_name = to_string(stage);

        char info_log[1024];
        glGetShaderInfoLog(gl_id, sizeof(info_log), null, info_log);
        log(LOG_IDENT_GL, LOG_ERROR, "Failed to compile %S shader %u", stage_name, index);
        if (info_log[0]) log(LOG_IDENT_GL, LOG_ERROR, "%s", info_log);
    }

    return index;
}

u32 gpu_new_framebuffer(const Gpu_Image_Format *color_formats, u32 color_format_count, u32 color_width, u32 color_height,
                        Gpu_Image_Format depth_format, u32 depth_width, u32 depth_height) {
    const auto index = gpu_next_index(gpu.framebuffers, gpu.free_framebuffers);

    auto &framebuffer = gpu.framebuffers[index];
    auto &gl_id       = framebuffer.handle._u32; Assert(!gl_id);

    framebuffer.color_attachments = New(u32, color_format_count);
    
    glCreateFramebuffers(1, &gl_id);
    
    for (u32 i = 0; i < color_format_count; ++i) {
        const auto format = color_formats[i];
        const auto image = gpu_new_image(GPU_IMAGE_TYPE_2D, format, 1, color_width, color_height, 1, {});
        const auto image_view = gpu_new_image_view(image, GPU_IMAGE_TYPE_2D, format, 0, 1, 0, 1);
        const auto gpu_image_view = gpu_get_image_view(image_view);
        const auto gl_view_id = gpu_image_view->handle._u32;
        
        glNamedFramebufferTexture(gl_id, GL_COLOR_ATTACHMENT0 + i, gl_view_id, 0);
        
        framebuffer.color_attachments[i] = image_view;
    }

    {
        const auto format = depth_format;
        const auto image = gpu_new_image(GPU_IMAGE_TYPE_2D, format, 1,
                                         depth_width, depth_height, 1, {});
        const auto image_view = gpu_new_image_view(image, GPU_IMAGE_TYPE_2D,
                                                   format, 0, 1, 0, 1);
        const auto gpu_image_view = gpu_get_image_view(image_view);
        const auto gl_view_id = gpu_image_view->handle._u32;
        
        glNamedFramebufferTexture(gl_id, GL_DEPTH_STENCIL_ATTACHMENT, gl_view_id, 0);

        framebuffer.depth_attachment = image_view;
    }

    const auto status = glCheckNamedFramebufferStatus(gl_id, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        log(LOG_IDENT_GL, LOG_ERROR, "[0x%X] Framebuffer %u is not complete", status, gl_id);
    }

    return index;
}

u32 gpu_new_command_buffer(u32 capacity) {
    const auto index = gpu_next_index(gpu.cmd_buffers, gpu.free_cmd_buffers);

    auto &cmd_buffer = gpu.cmd_buffers[index];
    auto &gl_id      = cmd_buffer.handle._u32; Assert(!gl_id); // opengl does not support command buffers

    cmd_buffer.commands = New(Gpu_Command, capacity);
    cmd_buffer.count    = 0;
    cmd_buffer.capacity = capacity;

    return index;
}

u32 gpu_new_vertex_input(const Gpu_Vertex_Binding *bindings, u32 binding_count,
                         const Gpu_Vertex_Attribute *attributes, u32 attribute_count) {
    const auto write_buffer = gpu_get_buffer(gpu_write_allocator.buffer);
    const auto index = gpu_next_index(gpu.vertex_inputs, gpu.free_vertex_inputs);

    auto &vertex_input = gpu.vertex_inputs[index];
    auto &gl_id        = vertex_input.handle._u32; Assert(!gl_id);

    glCreateVertexArrays(1, &gl_id);

    for (u32 i = 0; i < binding_count; ++i) {
        const auto &binding     = bindings[i];
        const auto advance_rate = to_gl_advance_rate(binding.input_rate);
        
        glVertexArrayBindingDivisor(gl_id, binding.index, advance_rate);
    }
    
    for (u32 i = 0; i < attribute_count; ++i) {
        const auto &attribute = attributes[i];
        const auto &binding   = bindings[attribute.binding];
        Assert(attribute.binding == binding.index);

        const auto gl_data_type = to_gl_vertex_data_type(attribute.type);
        const auto dimension    = gpu_vertex_attribute_dimension(attribute.type);

        glEnableVertexArrayAttrib (gl_id, attribute.index);
        glVertexArrayAttribBinding(gl_id, attribute.index, attribute.binding);
        
        if (attribute.type == GPU_VERTEX_ATTRIBUTE_TYPE_S32 ||
            attribute.type == GPU_VERTEX_ATTRIBUTE_TYPE_U32) {
            glVertexArrayAttribIFormat(gl_id, attribute.index, dimension, gl_data_type, attribute.offset);
        } else {
            const auto normalize = false;
            glVertexArrayAttribFormat(gl_id, attribute.index, dimension, gl_data_type, normalize, attribute.offset);
        }
    }
    
    vertex_input.bindings        = New(Gpu_Vertex_Binding,   binding_count);
    vertex_input.attributes      = New(Gpu_Vertex_Attribute, attribute_count);
    vertex_input.binding_count   = binding_count;
    vertex_input.attribute_count = attribute_count;

    copy(vertex_input.bindings,   bindings,   binding_count * sizeof(bindings[0]));
    copy(vertex_input.attributes, attributes, attribute_count * sizeof(attributes[0]));

    return index;
}

u32 gpu_new_descriptor(const Gpu_Descriptor_Binding *bindings, u32 count) {
    const auto index = gpu_next_index(gpu.descriptors, gpu.free_descriptors);

    auto &descriptor = gpu.descriptors[index];
    auto &gl_id      = descriptor.handle._u32; Assert(!gl_id); // opengl does not have descriptors

    descriptor.bindings      = New(Gpu_Descriptor_Binding, count);
    descriptor.binding_count = count;
    
    copy(descriptor.bindings, bindings, count * sizeof(bindings[0]));
    
    return index;
}

u32 gpu_new_pipeline(u32 vertex_input, u32 *shaders, u32 shader_count, u32 *descriptors, u32 descriptor_count) {
    const auto index = gpu_next_index(gpu.pipelines, gpu.free_pipelines);

    auto &pipeline = gpu.pipelines[index];
    auto &gl_id    = pipeline.handle._u32; Assert(!gl_id);

    gl_id = glCreateProgram();

    for (u32 i = 0; i < shader_count; ++i) {
        const auto &shader = gpu.shaders[shaders[i]];
        glAttachShader(gl_id, shader.handle._u32);
    }

    glLinkProgram(gl_id);

	s32 success;
	glGetProgramiv(gl_id, GL_LINK_STATUS, &success);
    
	if (!success) {
		char info_log[1024];
		glGetProgramInfoLog(gl_id, sizeof(info_log), null, info_log);        
		if (info_log[0]) log(LOG_IDENT_GL, LOG_ERROR, "Failed to link program %u", index);
	}

    pipeline.vertex_input     = vertex_input;
    pipeline.shaders          = New(u32, shader_count);
    pipeline.shader_count     = shader_count;
    pipeline.descriptors      = New(u32, descriptor_count);
    pipeline.descriptor_count = descriptor_count;

    copy(pipeline.shaders, shaders, shader_count * sizeof(shaders[0]));
    copy(pipeline.descriptors, descriptors, descriptor_count * sizeof(descriptors[0]));
    
    return index;
}

template <typename T, typename I>
static void gpu_delete_gl_resource(u32 index, Array <T> &resources, Array <I> &free_resources, void (*gl_delete)(GLsizei n, const GLuint *ids), void (*gl_delete_one)(GLuint id) = null) {
    if (!index || index >= resources.count) return;

    auto &resource = resources[index];
    auto &gl_id    = resource.handle._u32;

    if (gl_id) {
        if (gl_delete)     gl_delete(1, &gl_id);
        if (gl_delete_one) gl_delete_one(gl_id);
        gl_id = 0;
    }

    array_add(free_resources, index);
}

void gpu_delete_buffer         (u32 index) { gpu_delete_gl_resource(index, gpu.buffers, gpu.free_buffers, glDeleteBuffers); }
void gpu_delete_image          (u32 index) { gpu_delete_gl_resource(index, gpu.images, gpu.free_images, glDeleteTextures); }
void gpu_delete_image_view     (u32 index) { gpu_delete_gl_resource(index, gpu.image_views, gpu.free_image_views, glDeleteTextures); }
void gpu_delete_sampler        (u32 index) { gpu_delete_gl_resource(index, gpu.samplers, gpu.free_samplers, glDeleteSamplers); }
void gpu_delete_shader         (u32 index) { gpu_delete_gl_resource(index, gpu.shaders,  gpu.free_shaders, null, glDeleteShader); }
void gpu_delete_command_buffer (u32 index) { gpu_delete_gl_resource(index, gpu.cmd_buffers, gpu.free_cmd_buffers, null); }
void gpu_delete_vertex_input   (u32 index) { gpu_delete_gl_resource(index, gpu.vertex_inputs, gpu.free_vertex_inputs, glDeleteVertexArrays); }
void gpu_delete_descriptor     (u32 index) { gpu_delete_gl_resource(index, gpu.descriptors, gpu.free_descriptors, null); }
void gpu_delete_pipeline       (u32 index) { gpu_delete_gl_resource(index, gpu.pipelines, gpu.free_pipelines, null, glDeleteProgram); }

void gpu_delete_framebuffer(u32 index) {
    auto &framebuffer = gpu.framebuffers[index];

    for (u32 i = 0; i < framebuffer.color_attachment_count; ++i) {
        const auto attachment = framebuffer.color_attachments[i];
        const auto image_view = gpu_get_image_view(attachment);

        gpu_delete_image_view(attachment);
        gpu_delete_image(image_view->image);
    }

    framebuffer.color_attachment_count = 0;
    
    if (framebuffer.depth_attachment) {
        const auto attachment = framebuffer.depth_attachment;
        const auto image_view = gpu_get_image_view(attachment);

        gpu_delete_image_view(attachment);
        gpu_delete_image(image_view->image);
    }

    gpu_delete_gl_resource(index, gpu.buffers, gpu.free_buffers, glDeleteFramebuffers);
}

Handle gpu_fence_sync() {
    Handle sync = {};
    sync._ptr = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    return sync;
}

static Gpu_Sync_Result to_gpu_sync_result(u32 gl_wait_result) {
    switch (gl_wait_result) {
    case GL_ALREADY_SIGNALED:         return GPU_SYNC_RESULT_ALREADY_SIGNALED;
    case GL_TIMEOUT_EXPIRED:          return GPU_SYNC_RESULT_TIMEOUT_EXPIRED;
    case GL_CONDITION_SATISFIED:      return GPU_SYNC_RESULT_SIGNALED;
    case GL_WAIT_FAILED:              return GPU_SYNC_RESULT_WAIT_FAILED;
    default: unreachable_code_path(); return GPU_SYNC_RESULT_WAIT_FAILED;
    }
}

Gpu_Sync_Result wait_client_sync(Handle sync, u64 timeout) {
    auto result = glClientWaitSync((GLsync)sync._ptr, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    return to_gpu_sync_result(result);
}

void wait_gpu_sync(Handle sync) {
    glWaitSync((GLsync)sync._ptr, 0, GL_TIMEOUT_IGNORED);
}

void delete_gpu_sync(Handle sync) {
    glDeleteSync((GLsync)sync._ptr);
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

// Texture *new_texture(const Loaded_Texture &loaded_texture) {
//     auto path = copy_string(loaded_texture.path);
//     auto name = get_file_name_no_ext(path);
    
//     const s32 type            = GL_texture_types[loaded_texture.type];
//     const s32 level           = 0;
//     const s32 format          = GL_texture_formats[loaded_texture.format];
//     const s32 internal_format = GL_texture_internal_formats[loaded_texture.format];
//     const s32 border          = 0;
//     const s32 data_type       = GL_texture_data_types[loaded_texture.format];

//     Handle handle = {};
//     glCreateTextures(type, 1, &handle._u32);

//     if (!handle) {
//         log(LOG_ERROR, "Failed to create GL texture %u for %S", handle._u32, path);
//         return global_textures.error;
//     }

//     const s32 unpack_alignment = GL_unpack_alignments[loaded_texture.format];
//     glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    
//     const auto &contents = loaded_texture.contents;
//     glBindTexture(type, handle._u32);
//     // @Todo: properly set data based on texture type.
//     glTexImage2D(type, level, internal_format, loaded_texture.width,
//                  loaded_texture.height, border, format, data_type, contents.data);
//     glBindTexture(type, 0);

//     auto &texture = texture_table[name];
//     if (texture.handle) {
//         glDeleteTextures(1, &texture.handle._u32);
//     }

//     // @Todo: set texture address.

//     texture.path   = path;
//     texture.name   = name;
//     texture.handle = handle;
//     texture.width  = loaded_texture.width;
//     texture.height = loaded_texture.height;
//     texture.type   = loaded_texture.type;
//     texture.format = loaded_texture.format;

//     texture.wrap       = TEX_REPEAT;
//     texture.min_filter = TEX_NEAREST_MIPMAP_LINEAR;
//     texture.mag_filter = TEX_NEAREST;

//     const s32 wrap       = GL_texture_wraps  [texture.wrap];
//     const s32 min_filter = GL_texture_filters[texture.min_filter];
//     const s32 mag_filter = GL_texture_filters[texture.mag_filter];

//     glTextureParameteri(handle._u32, GL_TEXTURE_WRAP_S, wrap);
//     glTextureParameteri(handle._u32, GL_TEXTURE_WRAP_T, wrap);

//     glTextureParameteri(handle._u32, GL_TEXTURE_MIN_FILTER, min_filter);
//     glTextureParameteri(handle._u32, GL_TEXTURE_MAG_FILTER, mag_filter);

//     glGenerateTextureMipmap(handle._u32);

//     glPixelStorei(GL_UNPACK_ALIGNMENT, GL_default_unpack_alignment);

//     return &texture;
// }

// void update_texture_filters(Texture &texture, Texture_Filter_Type min_filter, Texture_Filter_Type mag_filter) {
//     texture.min_filter = min_filter;
//     texture.mag_filter = mag_filter;

//     const s32 gl_min_filter = GL_texture_filters[texture.min_filter];
//     const s32 gl_mag_filter = GL_texture_filters[texture.mag_filter];
    
//     glTextureParameteri(texture.handle._u32, GL_TEXTURE_MIN_FILTER, gl_min_filter);
//     glTextureParameteri(texture.handle._u32, GL_TEXTURE_MAG_FILTER, gl_mag_filter);
// }
