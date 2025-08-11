#include "pch.h"
#include "render/glad.h"
#include "render/render.h"
#include "render/r_table.h"
#include "render/r_pass.h"
#include "render/r_shader.h"
#include "render/r_uniform.h"
#include "render/r_material.h"
#include "render/r_texture.h"
#include "render/r_storage.h"
#include "render/r_stats.h"
#include "render/r_mesh.h"
#include "render/r_viewport.h"
#include "render/r_command.h"
#include "render/r_target.h"
#include "render/r_vertex_descriptor.h"

#include "os/file.h"

#include "math/math_basic.h"

#include "log.h"
#include "str.h"
#include "font.h"
#include "asset.h"

#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_sprintf.h"

struct GL_Indirect_Command {
    u32 count = 0;
    u32 instance_count = 0;
    u32 first = 0;
    u32 base_instance = 0;
};

struct GL_Indirect_Command_Indexed {
    u32 count = 0;
    u32 instance_count = 0;
    u32 first = 0;
    u32 base_vertex = 0;
    u32 base_instance = 0;
};

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

    // R_Vertex_Descriptor::MAX_BINDINGS index is used for eid vertex descriptor binding.
    Assert(R_Vertex_Descriptor::MAX_BINDINGS + 1 <= R_MAX_VERTEX_ATTRIBUTES);
                    
    log("Render capabilities:");
    log("  Texture:\n    Max Size %dx%d texels", R_MAX_TEXTURE_TEXELS, R_MAX_TEXTURE_TEXELS);
    log("  Uniform buffer:\n    Max Bindings %d\n    Base Alignment %d\n    Offset Alignment %d",
        R_MAX_UNIFORM_BUFFER_BINDINGS, R_UNIFORM_BUFFER_BASE_ALIGNMENT, R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    log("  Uniform block:\n    Max Size %d bytes", R_MAX_UNIFORM_BLOCK_SIZE);
    log("  Vertex:\n    Max Attributes %d", R_MAX_VERTEX_ATTRIBUTES);
}

#define GL_VALUE(x) to_gl_value(x)
static constexpr u32 to_gl_value(u32 v) {
    switch (v) {
    case R_ENABLE:  return GL_TRUE;
	case R_DISABLE: return GL_FALSE;
        
    case R_TRIANGLES:      return GL_TRIANGLES;
	case R_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
	case R_LINES:          return GL_LINES;

    case R_FILL:  return GL_FILL;
	case R_LINE:  return GL_LINE;
	case R_POINT: return GL_POINT;

    case R_CW:  return GL_CW;
	case R_CCW: return GL_CCW;

    case R_BACK:       return GL_BACK;
	case R_FRONT:      return GL_FRONT;
	case R_FRONT_BACK: return GL_FRONT_AND_BACK;

    case R_SRC_ALPHA:           return GL_SRC_ALPHA;
	case R_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;

    case R_LESS:    return GL_LESS;
    case R_GREATER: return GL_GREATER;
    case R_EQUAL:   return GL_EQUAL;
    case R_NEQUAL:  return GL_NOTEQUAL;
    case R_LEQUAL:  return GL_LEQUAL;
    case R_GEQUAL:  return GL_GEQUAL;

    case R_ALWAYS:  return GL_ALWAYS;
    case R_KEEP:    return GL_KEEP;
	case R_REPLACE: return GL_REPLACE;

    case R_S32:     return GL_INT;
    case R_U32:     return GL_UNSIGNED_INT;
    case R_F32:     return GL_FLOAT;
    case R_F32_2:   return GL_FLOAT;
    case R_F32_3:   return GL_FLOAT;
    case R_F32_4:   return GL_FLOAT;
    case R_F32_4X4: return GL_FLOAT;
        
    case R_TEXTURE_2D:       return GL_TEXTURE_2D;
    case R_TEXTURE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;

    case R_RED_32:             return GL_R32I;
    case R_RGB_8:              return GL_RGB8;
    case R_RGBA_8:             return GL_RGBA8;
    case R_DEPTH_24_STENCIL_8: return GL_DEPTH24_STENCIL8;

    case R_REPEAT: return GL_REPEAT;
    case R_CLAMP:  return GL_CLAMP_TO_EDGE;
    case R_BORDER: return GL_CLAMP_TO_BORDER;

    case R_LINEAR:                 return GL_LINEAR;
    case R_NEAREST:                return GL_NEAREST;
    case R_LINEAR_MIPMAP_LINEAR:   return GL_LINEAR_MIPMAP_LINEAR;
    case R_LINEAR_MIPMAP_NEAREST:  return GL_LINEAR_MIPMAP_NEAREST;
    case R_NEAREST_MIPMAP_LINEAR:  return GL_NEAREST_MIPMAP_LINEAR;
    case R_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;

    default:
        error("Failed to get gl value from %u", v);
        return 0;
    }
}

#define GL_CLEAR_BITS(x) to_gl_clear_bits(x)
static constexpr u32 to_gl_clear_bits(u32 bits) {
    u32 gl_bits = 0;

    if (bits & R_COLOR_BUFFER_BIT)   gl_bits |= GL_COLOR_BUFFER_BIT;
    if (bits & R_DEPTH_BUFFER_BIT)   gl_bits |= GL_DEPTH_BUFFER_BIT;
    if (bits & R_STENCIL_BUFFER_BIT) gl_bits |= GL_STENCIL_BUFFER_BIT;
    
    return gl_bits;
}

void r_submit(const R_Target &target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.rid);
}

void r_submit(const R_Pass &pass) {
    if (pass.polygon != R_NONE) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_VALUE(pass.polygon));
    }

    if (pass.viewport.w > 0 && pass.viewport.h > 0) {
        glViewport(pass.viewport.x, pass.viewport.y, pass.viewport.w, pass.viewport.h);
    }

    if (pass.scissor.test == R_ENABLE) {
        glEnable(GL_SCISSOR_TEST);
    } else if (pass.scissor.test == R_DISABLE) {
        glDisable(GL_SCISSOR_TEST);
    }

    if (pass.scissor.w > 0 && pass.scissor.h > 0) {
        glScissor(pass.scissor.x, pass.scissor.y, pass.scissor.w, pass.scissor.h);
    }
    
    if (pass.cull.test == R_ENABLE) {
        glEnable(GL_CULL_FACE);
    } else if (pass.cull.test == R_DISABLE) {
        glDisable(GL_CULL_FACE);
    }

    if (pass.cull.face != R_NONE) {
        glCullFace(GL_VALUE(pass.cull.face));
    }

    if (pass.cull.winding != R_NONE) {
        glFrontFace(GL_VALUE(pass.cull.winding));
    }
        
    if (pass.blend.test == R_ENABLE) {
        glEnable(GL_BLEND);
    } else if (pass.blend.test == R_DISABLE) {
        glDisable(GL_BLEND);
    }

    if (pass.blend.src != R_NONE && pass.blend.dst != R_NONE) {
        glBlendFunc(GL_VALUE(pass.blend.src),
                    GL_VALUE(pass.blend.dst));
    }
            
    if (pass.depth.test == R_ENABLE) {
        glEnable(GL_DEPTH_TEST);
    } else if (pass.depth.test == R_DISABLE) {
        glDisable(GL_DEPTH_TEST);
    }

    if (pass.depth.func != R_NONE) {
        glDepthFunc(GL_VALUE(pass.depth.func));
    }

    if (pass.depth.mask != R_NONE) {
        glDepthMask(GL_VALUE(pass.depth.mask));
    }
        
    if (pass.stencil.test == R_ENABLE) {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(pass.stencil.mask);
    } else if (pass.stencil.test == R_DISABLE) {
        glStencilMask(pass.stencil.mask);
        glDisable(GL_STENCIL_TEST);    
    }
    
    if (pass.stencil.op.stencil_failed != R_NONE
        && pass.stencil.op.depth_failed != R_NONE
        && pass.stencil.op.passed != R_NONE) {
        glStencilOp(GL_VALUE(pass.stencil.op.stencil_failed),
                    GL_VALUE(pass.stencil.op.depth_failed),
                    GL_VALUE(pass.stencil.op.passed));
    }

    if (pass.stencil.func.type != R_NONE) {
        glStencilFunc(GL_VALUE(pass.stencil.func.type),
                      pass.stencil.func.comparator,
                      pass.stencil.func.mask);
    }
        
    if (pass.clear.bits != R_NONE) {
        glClearColor(rgba_get_r(pass.clear.color) / 255.0f,
                     rgba_get_g(pass.clear.color) / 255.0f,
                     rgba_get_b(pass.clear.color) / 255.0f,
                     rgba_get_a(pass.clear.color) / 255.0f);
        glClear(GL_CLEAR_BITS(pass.clear.bits));
    }        
}

static void r_write_uniform_to_gpu(rid shader, const char *name, u16 type, u32 count, const void *data) {
    const s32 location = glGetUniformLocation(shader, name);
    if (location < 0) {
        return;
    }
    
    switch (type) {
	case R_U32:     glProgramUniform1uiv(shader, location, count, (u32 *)data); break;
    case R_F32:     glProgramUniform1fv(shader,  location, count, (f32 *)data); break;
    case R_F32_2:   glProgramUniform2fv(shader,  location, count, (f32 *)data); break;
    case R_F32_3:   glProgramUniform3fv(shader,  location, count, (f32 *)data); break;
    case R_F32_4:   glProgramUniform4fv(shader,  location, count, (f32 *)data); break;
	case R_F32_4X4: glProgramUniformMatrix4fv(shader, location, count, GL_FALSE, (f32 *)data); break;
	default:
		error("Failed to write uniform %s of unknown type %d", name, type);
		break;
	}
}

static s32 cb_compare_command_list_queued(const void *a, const void *b) {
    const auto &qa = *(const R_Command_List::Queued *)a;
    const auto &qb = *(const R_Command_List::Queued *)b;

    const u64 ska = qa.sort_key._u64;
    const u64 skb = qb.sort_key._u64;
    
    if (ska < skb) return -1;
    if (ska > skb) return  1;
    
    return 0;
}

bool operator<(const R_Command_List::Queued &a, const R_Command_List::Queued &b) {
    return a.sort_key._u64 < b.sort_key._u64;;
}

#include <algorithm>

void r_submit(R_Command_List &list) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    //sort(list.queued, list.count, sizeof(list.queued[0]), cb_compare_command_list_queued);

    // @Todo: implement stable sort.
    std::stable_sort(list.queued, list.queued + list.count);
    
    u32 indirect_count = 0;
    u32 indexed_indirect_count = 0;
    
    auto *indirects = arena_push_array(scratch.arena, list.capacity, GL_Indirect_Command);
    auto *indexed_indirects = arena_push_array(scratch.arena, list.capacity, GL_Indirect_Command_Indexed);

    for (u32 i = 0; i < list.count; ++i) {
        const auto &cmd = list.queued[i].command;

        if (cmd.bits & R_CMD_INDEXED_BIT) {
            GL_Indirect_Command_Indexed indirect;
            indirect.count = cmd.count;
            indirect.instance_count = cmd.instance_count;
            indirect.first = cmd.first;
            indirect.base_vertex = 0;
            indirect.base_instance = cmd.base_instance;
            
            indexed_indirects[indexed_indirect_count] = indirect;
            indexed_indirect_count += 1;
        } else {
            GL_Indirect_Command indirect;
            indirect.count = cmd.count;
            indirect.instance_count = cmd.instance_count;
            indirect.first = cmd.first;
            indirect.base_instance = cmd.base_instance;
            
            indirects[indirect_count] = indirect;
            indirect_count += 1;
        }
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer_indexed);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
                    indexed_indirect_count * sizeof(GL_Indirect_Command_Indexed),
                    indexed_indirects);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
                    indirect_count * sizeof(GL_Indirect_Command),
                    indirects);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
 
    u32 render_count = 0;
    u32 rendered_indirect_count = 0;
    u32 rendered_indexed_indirect_count = 0;

    // @Cleanup: this loop looks kinda bad.
    for (u32 i = 0; i < list.count; i += render_count) {
        const auto &cmd = list.queued[i].command;
        const auto gl_mode = GL_VALUE(cmd.mode);

        const rid gl_shader = R_table.shaders[cmd.shader].rid;
        const rid gl_texture = R_table.textures[cmd.texture].rid;
        const rid gl_vd = R_table.vertex_descriptors[cmd.vertex_desc].rid;

        glUseProgram(gl_shader);
        glBindVertexArray(gl_vd);
        glBindTextureUnit(0, gl_texture);
 
#if DEVELOPER
        // Add extra binding for entity id.
        glVertexArrayVertexBuffer(gl_vd, EID_VERTEX_BINDING_INDEX,
                                  R_vertex_map_range.storage->rid,
                                  cmd.eid_offset, sizeof(u32));
        glEnableVertexArrayAttrib(gl_vd, EID_VERTEX_BINDING_INDEX);
        glVertexArrayAttribBinding(gl_vd, EID_VERTEX_BINDING_INDEX, EID_VERTEX_BINDING_INDEX);
        glVertexArrayAttribIFormat(gl_vd, EID_VERTEX_BINDING_INDEX, 1, GL_UNSIGNED_INT, 0);
        glVertexArrayBindingDivisor(gl_vd, EID_VERTEX_BINDING_INDEX, 1);
#endif

        render_count = 0;
        for (u32 j = i; j < list.count; ++j) {
            const auto &ncmd = list.queued[j].command;
            
            if (cmd.bits != ncmd.bits
                || cmd.mode != ncmd.mode
                || cmd.shader != ncmd.shader
                || cmd.vertex_desc != ncmd.vertex_desc
                || cmd.texture != ncmd.texture) {
                break;
            }

            render_count += 1;
        }

        // @Todo: this will NOT work in case of actual multidraw with count > 1.
        for (u32 j = 0; j < cmd.uniform_count; ++j) {
            const auto &u = R_table.uniforms[cmd.uniforms[j]];
            const String name = sid_str(u.name);
            const void *data = (u8 *)R_table.uniform_value_cache.data + u.offset;

            char cname[u.MAX_NAME_LENGTH];
            str_c(name, COUNT(cname), cname);
            
            r_write_uniform_to_gpu(gl_shader, cname, u.type, u.count, data);
        }
        
        if (cmd.bits & R_CMD_INDEXED_BIT) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer_indexed);

            const u64 offset = rendered_indexed_indirect_count * sizeof(GL_Indirect_Command_Indexed);
            glMultiDrawElementsIndirect(gl_mode, GL_UNSIGNED_INT,
                                        (void *)offset, render_count, 0);
            
            rendered_indexed_indirect_count += render_count;
        } else {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer);

            const u64 offset = rendered_indirect_count * sizeof(GL_Indirect_Command);
            glMultiDrawArraysIndirect(gl_mode, (void *)offset, render_count, 0);
            
            rendered_indirect_count += render_count;
        }

        Draw_call_count += 1;
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    list.count = 0;
}

void r_create_command_list(u32 capacity, R_Command_List &list) {
    Assert(list.queued == null);
    Assert(list.command_buffer == RID_NONE);
    Assert(list.command_buffer_indexed == RID_NONE);

    // @Cleanup: use own arena?
    list.queued = arena_push_array(M_global, capacity, R_Command_List::Queued);
    list.count = 0;
    list.capacity = capacity;

    glGenBuffers(1, &list.command_buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, capacity * sizeof(GL_Indirect_Command),
                 0, GL_STREAM_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    glGenBuffers(1, &list.command_buffer_indexed);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, list.command_buffer_indexed);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, capacity * sizeof(GL_Indirect_Command_Indexed),
                 0, GL_STREAM_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

u16 r_create_render_target(u16 w, u16 h, u16 count, const u16 *cformats, u16 dformat) {
    Assert(count <= R_Target::MAX_COLOR_ATTACHMENTS);

    R_Target rt;
    
    glGenFramebuffers(1, &rt.rid);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.rid);

    rt.color_attachment_count = count;
    
    u32 gl_color_attachments[rt.MAX_COLOR_ATTACHMENTS];
    for (u8 i = 0; i < count; ++i) {
        gl_color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        rt.color_attachments[i] = R_Attachment { 0, cformats[i] };
    }
    
    glDrawBuffers(count, gl_color_attachments);
    
    rt.depth_attachment.texture = 0;
    rt.depth_attachment.format  = dformat;

    const u16 target = sparse_push(R_table.targets, rt);
    r_resize_render_target(target, w, h);

    return target;
}

void r_resize_render_target(u16 target, u16 w, u16 h) {
    auto &rt = R_table.targets[target];

    rt.width  = w;
    rt.height = h;

    glBindFramebuffer(GL_FRAMEBUFFER, rt.rid);	
    
    for (u32 i = 0; i < rt.color_attachment_count; ++i) {
        auto &attachment = rt.color_attachments[i];
        if (attachment.texture != 0) {
            r_delete_texture(attachment.texture);
            attachment.texture = 0;
        }

        attachment.texture = r_create_texture(R_TEXTURE_2D, attachment.format, w, h,
                                              R_BORDER, R_NEAREST, R_NEAREST);

        const auto &tx = R_table.textures[attachment.texture];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D, tx.rid, 0);        
    }

    {
        auto &attachment = rt.depth_attachment;
        if (attachment.texture != 0) {
            r_delete_texture(attachment.texture);
            attachment.texture = 0;
        }

        attachment.texture = r_create_texture(R_TEXTURE_2D, attachment.format, w, h,
                                              R_BORDER, R_NEAREST, R_NEAREST);

        const auto &tx = R_table.textures[attachment.texture];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D, tx.rid, 0);
    }
        
    const u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        warn("Framebuffer %u is not complete, GL status 0x%X", rt.rid, status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

s32 r_read_render_target_pixel(u16 target, u32 color_attachment_index, u32 x, u32 y) {
    const auto &rt = R_table.targets[target];
    glBindFramebuffer(GL_FRAMEBUFFER, rt.rid);
    const s32 pixel = r_read_pixel(color_attachment_index, x, y);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return pixel;
}

s32 r_read_pixel(u32 color_attachment_index, u32 x, u32 y) {
    glReadBuffer(GL_COLOR_ATTACHMENT0 + color_attachment_index);

    s32 pixel = 0;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixel);

    return pixel;
}

u16 fb_vd = 0;
u32 fb_first_index = 0;
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

    R_Vertex_Binding binding = {};
    binding.binding_index = 0;
    binding.offset = R_vertex_map_range.offset + R_vertex_map_range.size;
    binding.component_count = 2;
    binding.components[0] = { R_F32_2, 0 };
    binding.components[1] = { R_F32_2, 0 };

    void *vertex_data = r_alloc(R_vertex_map_range, sizeof(vertices)).data;
    mem_copy(vertex_data, vertices, sizeof(vertices));
    
    fb_vd = r_create_vertex_descriptor(1, &binding);

    const u32 indices[6] = { 0, 2, 1, 2, 0, 3 };
    fb_index_count = COUNT(indices);
    fb_first_index = (R_index_map_range.offset + R_index_map_range.size) / sizeof(u32);

    void *index_data = r_alloc(R_index_map_range, sizeof(indices)).data;
    mem_copy(index_data, indices, sizeof(indices));
}

#define GL_STORAGE_BITS(x) to_gl_storage_bits(x)
static u32 to_gl_storage_bits(u32 bits) {
    u32 gl_bits = 0;

    if (bits & R_DYNAMIC_STORAGE_BIT) gl_bits |= GL_DYNAMIC_STORAGE_BIT;
    if (bits & R_CLIENT_STORAGE_BIT)  gl_bits |= GL_CLIENT_STORAGE_BIT;
    
    if (bits & R_MAP_READ_BIT)        gl_bits |= GL_MAP_READ_BIT;
    if (bits & R_MAP_WRITE_BIT)       gl_bits |= GL_MAP_WRITE_BIT;
    if (bits & R_MAP_PERSISTENT_BIT)  gl_bits |= GL_MAP_PERSISTENT_BIT;
    if (bits & R_MAP_COHERENT_BIT)    gl_bits |= GL_MAP_COHERENT_BIT;

    return gl_bits;
}

#define GL_MAP_BITS(x) to_gl_map_bits(x)
static u32 to_gl_map_bits(u32 bits) {
    u32 gl_bits = 0;

    if (bits & R_MAP_READ_BIT)       gl_bits |= GL_MAP_READ_BIT;
    if (bits & R_MAP_WRITE_BIT)      gl_bits |= GL_MAP_WRITE_BIT;
    if (bits & R_MAP_PERSISTENT_BIT) gl_bits |= GL_MAP_PERSISTENT_BIT;
    if (bits & R_MAP_COHERENT_BIT)   gl_bits |= GL_MAP_COHERENT_BIT;

    return gl_bits;
}

void r_create_storage(u32 size, u32 bits, R_Storage &storage) {    
    rid rid = RID_NONE;
    glCreateBuffers(1, &rid);
    glNamedBufferStorage(rid, size, null, GL_STORAGE_BITS(bits));

    storage.rid = rid;
    storage.bits = bits;
    storage.size = size;
}

R_Map_Range r_map(R_Storage &storage, u32 offset, u32 size, u32 bits) {
    Assertm((storage.bits & R_MAPPED_STORAGE_BIT) == 0, "Given storage is already mapped");
    Assert(offset + size <= storage.size);

    storage.bits |= R_MAPPED_STORAGE_BIT;
    
    R_Map_Range range;
    range.storage = &storage;
    range.bits = bits;
    range.offset = offset;
    range.size = 0;
    range.capacity = size;
    range.data = glMapNamedBufferRange(storage.rid, offset, size, GL_MAP_BITS(bits));

    return range;
}

bool r_unmap(R_Storage &storage) {
    storage.bits &= ~R_MAPPED_STORAGE_BIT;
    return glUnmapNamedBuffer(storage.rid);
}

void r_flush(const R_Map_Range &map) {
    glFlushMappedNamedBufferRange(map.storage->rid, map.offset, map.size);
}

u16 r_create_vertex_descriptor(u32 count, const R_Vertex_Binding *bindings) {
    R_Vertex_Descriptor vd;
    Assert(count <= vd.MAX_BINDINGS);

    vd.binding_count = count;
    mem_copy(vd.bindings, bindings, count * sizeof(bindings[0]));
    
    glCreateVertexArrays(1, &vd.rid);

    s32 attribute_index = 0;
    for (u32 binding_index = 0; binding_index < count; ++binding_index) {        
        const auto &binding = vd.bindings[binding_index];
        
        u32 vertex_size = 0;
        for (u32 j = 0; j < binding.component_count; ++j) {
            vertex_size += r_vertex_component_size(binding.components[j].type);
        }

        // @Todo: add storage ref to binding to pass here.
        glVertexArrayVertexBuffer(vd.rid, binding.binding_index,
                                  R_vertex_map_range.storage->rid,
                                  binding.offset, vertex_size);

        u32 offset = 0;
        for (u32 j = 0; j < binding.component_count; ++j) {
            const auto &component = binding.components[j];
            
            const s32 data_type = GL_VALUE(component.type);
            const s32 dimension = r_vertex_component_dimension(component.type);

            glEnableVertexArrayAttrib(vd.rid, attribute_index);
            glVertexArrayAttribBinding(vd.rid, attribute_index, binding_index);
            
            if (component.type == R_S32 || component.type == R_U32) {
                glVertexArrayAttribIFormat(vd.rid, attribute_index, dimension, data_type, offset);
            } else {
                glVertexArrayAttribFormat(vd.rid, attribute_index, dimension, data_type, component.normalize, offset);
            }

            glVertexArrayBindingDivisor(vd.rid, attribute_index, component.advance_rate);

            offset += r_vertex_component_size(component.type);
            attribute_index += 1;
        }
	}

    // Bind global index buffer.
    glBindVertexArray(vd.rid);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, R_index_map_range.storage->rid);
    glBindVertexArray(0);
    
    return sparse_push(R_table.vertex_descriptors, vd);
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
		error("Failed to compile %s shader, gl reason %s", shader_name, info_log);
		return INDEX_NONE;
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
		return INDEX_NONE;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

static void r_create_gl_shader(String s, R_Shader &sh) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    char *vertex   = arena_push_array(scratch.arena, R_Shader::MAX_FILE_SIZE, char);
	char *fragment = arena_push_array(scratch.arena, R_Shader::MAX_FILE_SIZE, char);
    
	if (!parse_shader_regions(s.value, vertex, fragment)) {
        error("Failed to parse shader regions:\n%.*s\n", s.length, s.value);
        return;
    }

    const u32 gl_vertex = gl_create_shader(GL_VERTEX_SHADER, vertex);
    if (gl_vertex == GL_INVALID_INDEX) {
        return;
    }
    
	const u32 gl_fragment = gl_create_shader(GL_FRAGMENT_SHADER, fragment);
    if (gl_fragment == GL_INVALID_INDEX) {
        return;
    }

    const auto sh_rid = (rid)gl_link_program(gl_vertex, gl_fragment);
    if (sh_rid != RID_NONE) {
        if (sh.rid != RID_NONE) {
            glDeleteProgram(sh.rid);
        }

        sh.rid = sh_rid;

        // Set or update uniform block bindings.
        Assert(COUNT(UNIFORM_BLOCK_NAMES) == COUNT(UNIFORM_BLOCK_BINDINGS));
        for (s32 i = 0; i < COUNT(UNIFORM_BLOCK_NAMES); ++i) {
            const u32 index = glGetUniformBlockIndex(sh.rid, UNIFORM_BLOCK_NAMES[i]);
            if (index != GL_INVALID_INDEX) {
                glUniformBlockBinding(sh.rid, index, UNIFORM_BLOCK_BINDINGS[i]);
            }   
        }
    }
}

u16 r_create_shader(String s) {
    R_Shader sh;
    r_create_gl_shader(s, sh);
    return sparse_push(R_table.shaders, sh);
}

void r_recreate_shader(u16 shader, String s) {
    auto &sh = R_table.shaders[shader];
    r_create_gl_shader(s, sh);
}

// std140 alignment rules
static inline u32 r_uniform_block_field_offset_gpu_aligned(const Uniform_Block_Field *fields, s32 field_index, s32 field_element_index) {
    u32 offset = 0;
    for (s32 i = 0; i <= field_index; ++i) {
        const auto &field = fields[i];

        u32 stride = r_uniform_type_size_gpu_aligned(field.type);
        u32 alignment = r_uniform_type_alignment(field.type);
        
        if (field.count > 1) {
            alignment = R_UNIFORM_BUFFER_BASE_ALIGNMENT;
            stride = Alignup(stride, R_UNIFORM_BUFFER_BASE_ALIGNMENT);
        }

        offset = Alignup(offset, alignment);

        s32 element_count = field.count;
        if (i == field_index) {
            Assert(field_element_index < field.count);
            element_count = field_element_index;
        }
        
        offset += stride * element_count;
    }

    offset = Alignup(offset, R_UNIFORM_BUFFER_BASE_ALIGNMENT);

    return offset;
}

// std140 alignment rules
static inline u32 get_uniform_block_size_gpu_aligned(const Uniform_Block_Field *fields, s32 field_count) {
    u32 size = 0;
    for (s32 i = 0; i < field_count; ++i) {
        const auto &field = fields[i];

        u32 stride = r_uniform_type_size_gpu_aligned(field.type);
        u32 alignment = r_uniform_type_alignment(field.type);
        
        if (field.count > 1) {
            alignment = R_UNIFORM_BUFFER_BASE_ALIGNMENT;
            stride = Alignup(stride, R_UNIFORM_BUFFER_BASE_ALIGNMENT);
        }

        size = Alignup(size, alignment);
        size += stride * field.count;
    }

    size = Alignup(size, R_UNIFORM_BUFFER_BASE_ALIGNMENT);

    return size;
}

u32 r_uniform_type_size_gpu_aligned(u16 type) {
    constexpr u32 N = 4;
    
    switch (type) {
	case R_U32:     return N;
	case R_F32:     return N;
	case R_F32_2:   return N * 2;
	case R_F32_3:   return N * 4;
	case R_F32_4:   return N * 4;
	case R_F32_4X4: return N * 16;
    default:
        error("Failed to get uniform gpu aligned size from type %d", type);
        return 0;
    }
}

u32 r_uniform_type_alignment(u16 type) {
    constexpr u32 N = 4;

    switch (type) {
	case R_U32:     return N;
	case R_F32:     return N;
	case R_F32_2:   return N * 2;
	case R_F32_3:   return N * 4;
	case R_F32_4:   return N * 4;
	case R_F32_4X4: return N * 4;
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
    UNIFORM_BUFFER_SIZE = Alignup(UNIFORM_BUFFER_SIZE, R_UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    Assert(UNIFORM_BUFFER_SIZE <= MAX_UNIFORM_BUFFER_SIZE);
    
    glBindBufferRange(GL_UNIFORM_BUFFER, shader_binding, rid_uniform_buffer, offset, size);

    block->rid_uniform_buffer = rid_uniform_buffer;
    block->name = name;
    mem_copy(block->fields, fields, field_count * sizeof(Uniform_Block_Field));
    block->field_count = field_count;
    block->shader_binding = shader_binding;
    block->offset = offset;
    block->size = size;
}

void r_set_uniform_block_value(Uniform_Block *block, s32 field_index, s32 field_element_index, const void *data, u32 size) {
    const u32 field_offset = r_uniform_block_field_offset_gpu_aligned(block, field_index, field_element_index);
    r_set_uniform_block_value(block, field_offset, data, size);
}

void r_set_uniform_block_value(Uniform_Block *block, u32 offset, const void *data, u32 size) {
    Assert(offset + size <= block->size);
    glNamedBufferSubData(block->rid_uniform_buffer, block->offset + offset, size, data);
}

u32 r_uniform_block_field_size_gpu_aligned(const Uniform_Block_Field &field) {
    if (field.count == 1) {
        return r_uniform_type_size_gpu_aligned(field.type);
    }

    return r_uniform_type_size_gpu_aligned(R_F32_4) * field.count;
}

u32 r_uniform_block_field_offset_gpu_aligned(Uniform_Block *block, s32 field_index, s32 field_element_index) {
    Assert(field_index < block->field_count);
    Assert(field_element_index < block->fields[field_index].count);
    return r_uniform_block_field_offset_gpu_aligned(block->fields, field_index, field_element_index);
}

static s32 to_gl_format(u32 format) {
    switch (format) {
    case R_RED_32:             return GL_RED_INTEGER;
    case R_RGB_8:              return GL_RGB;
    case R_RGBA_8:             return GL_RGBA;
    case R_DEPTH_24_STENCIL_8: return GL_DEPTH_STENCIL;
    default:
        error("Failed to get GL format from format %d", format);
        return -1;
    }
}

static s32 to_gl_data_type(u32 format) {
    switch (format) {
    case R_RED_32:             return GL_INT;
    case R_RGB_8:              return GL_UNSIGNED_BYTE;
    case R_RGBA_8:             return GL_UNSIGNED_BYTE;
    case R_DEPTH_24_STENCIL_8: return GL_UNSIGNED_INT_24_8;
    default:
        error("Failed to get GL data type from format %d", format);
        return -1;
    }
}

static rid gl_create_texture(u16 type, u16 format, u16 w, u16 h, u16 wrap,
                             u16 min_filter, u16 mag_filter, void *data) {
    rid tx_rid = RID_NONE;
    const s32 gl_type = GL_VALUE(type);
    glCreateTextures(gl_type, 1, &tx_rid);

    if (tx_rid == RID_NONE) return RID_NONE;
    
    glBindTexture(gl_type, tx_rid);
    // @Todo: properly set data based on texture type.
    glTexImage2D(gl_type, 0,
                 GL_VALUE(format),
                 w, h, 0,
                 to_gl_format(format),
                 to_gl_data_type(format),
                 data);
    glBindTexture(gl_type, 0);

    const s32 gl_wrap = GL_VALUE(wrap);
    glTextureParameteri(tx_rid, GL_TEXTURE_WRAP_S, gl_wrap);
    glTextureParameteri(tx_rid, GL_TEXTURE_WRAP_T, gl_wrap);

    glTextureParameteri(tx_rid, GL_TEXTURE_MIN_FILTER, GL_VALUE(min_filter));
    glTextureParameteri(tx_rid, GL_TEXTURE_MAG_FILTER, GL_VALUE(mag_filter));

    glGenerateTextureMipmap(tx_rid);

    return tx_rid;
}

static void r_create_gl_texture(u16 type, u16 format, u16 w, u16 h, u16 wrap,
                                u16 min_filter, u16 mag_filter, void *data, R_Texture &tx) {
    const rid tx_rid = gl_create_texture(type, format, w, h, wrap,
                                         min_filter, mag_filter, data);
    if (tx_rid != RID_NONE) {
        if (tx.rid != RID_NONE) {
            glDeleteTextures(1, &tx.rid);
        }

        tx.rid = tx_rid;
        
        tx.width  = w;
        tx.height = h;
        tx.type   = type;
        tx.format = format;
        tx.wrap   = wrap;
        tx.min_filter = min_filter;
        tx.mag_filter = mag_filter;
    }
}

u16 r_create_texture(u16 type, u16 format, u16 w, u16 h, u16 wrap,
                     u16 min_filter, u16 mag_filter, void *data) {
    R_Texture tx;
    r_create_gl_texture(type, format, w, h, wrap, min_filter, mag_filter, data, tx);

    return sparse_push(R_table.textures, tx);
}

void r_recreate_texture(u16 texture, u16 type, u16 format, u16 w, u16 h, u16 wrap,
                        u16 min_filter, u16 mag_filter, void *data) {
    auto &tx = R_table.textures[texture];
    r_create_gl_texture(type, format, w, h, wrap, min_filter, mag_filter, data, tx);
}

void r_delete_texture(u16 texture) {
    auto &tx = R_table.textures[texture];
    glDeleteTextures(1, &tx.rid);

    tx = {};
    sparse_remove(R_table.textures, texture);
}

void rescale_font_atlas(Font_Atlas &atlas, s16 font_size) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    const auto &tx = R_table.textures[atlas.texture];
    
	const Font_Info *font = atlas.font;
	atlas.font_size = font_size;

	const u32 charcode_count = atlas.end_charcode - atlas.start_charcode + 1;
	const f32 scale = stbtt_ScaleForPixelHeight(font->info, (f32)font_size);
	atlas.px_h_scale = scale;
	atlas.line_height = (s32)((font->ascent - font->descent + font->line_gap) * scale);

	const s32 space_glyph_index = stbtt_FindGlyphIndex(font->info, ' ');
	stbtt_GetGlyphHMetrics(font->info, space_glyph_index, &atlas.space_advance_width, 0);
	atlas.space_advance_width = (s32)(atlas.space_advance_width * scale);

	s32 max_layers;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
	Assert(charcode_count <= (u32)max_layers);

	// stbtt rasterizes glyphs as 8bpp, so tell GL to use 1 byte per color channel.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D_ARRAY, tx.rid);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, atlas.font_size, atlas.font_size, charcode_count, 0, GL_RED, GL_UNSIGNED_BYTE, null);

	u8 *bitmap = arena_push_array(scratch.arena, font_size * font_size, u8);
        
	for (u32 i = 0; i < charcode_count; ++i) {
		const u32 c = i + atlas.start_charcode;
		Font_Glyph_Metric *metric = atlas.metrics + i;

		s32 w, h, offx, offy;
		const s32 glyph_index = stbtt_FindGlyphIndex(font->info, c);
		u8 *stb_bitmap = stbtt_GetGlyphBitmap(font->info, scale, scale, glyph_index, &w, &h, &offx, &offy);

		// Offset original bitmap to be at center of new one.
		const s32 x_offset = (font_size - w) / 2;
		const s32 y_offset = (font_size - h) / 2;

		metric->offset_x = offx - x_offset;
		metric->offset_y = offy - y_offset;

		mem_set(bitmap, 0, font_size * font_size);

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
