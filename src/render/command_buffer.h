#pragma once

#include "gpu.h"

enum Polygon_Mode : u8 {
    POLYGON_FILL,
    POLYGON_LINE,
    POLYGON_POINT,
};

enum Topology_Mode : u8 {
    TOPOLOGY_LINES,
    TOPOLOGY_TRIANGLES,
    TOPOLOGY_TRIANGLE_STRIP,
};

enum Cull_Face : u8 {
    CULL_FRONT,
    CULL_BACK,
    CULL_FRONT_AND_BACK,
};

enum Winding_Type : u8 {
    WINDING_CW,
    WINDING_CCW,
};

enum Blend_Func : u8 {
    BLEND_SRC_ALPHA,
    BLEND_ONE_MINUS_SRC_ALPHA,
};

enum Depth_Func : u8 {
    DEPTH_LESS,
    DEPTH_GREATER,
    DEPTH_EQUAL,
    DEPTH_NOT_EQUAL,
    DEPTH_LESS_EQUAL,
    DEPTH_GREATER_EQUAL,
};

enum Stencil_Func : u8 {
    STENCIL_ALWAYS,
    STENCIL_KEEP,
    STENCIL_REPLACE,
    STENCIL_NOT_EQUAL,
};

enum Clear_Bits : u32 {
    CLEAR_COLOR_BIT   = 0x1,
    CLEAR_DEPTH_BIT   = 0x2,
    CLEAR_STENCIL_BIT = 0x4,

    CLEAR_ALL_BITS = CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT,
};

enum Command_Type : u8 {
    CMD_NONE,
    CMD_POLYGON,
    CMD_TOPOLOGY,
    CMD_VIEWPORT,
    CMD_SCISSOR,
    CMD_CULL,
    CMD_BLEND_WRITE,
    CMD_BLEND_FUNC,
    CMD_DEPTH_WRITE,
    CMD_DEPTH_FUNC,
    CMD_STENCIL_MASK,
    CMD_STENCIL_FUNC,
    CMD_STENCIL_OP,
    CMD_CLEAR,
    CMD_SHADER,
    CMD_TEXTURE,
    CMD_VERTEX_DESCRIPTOR,
    CMD_VERTEX_BINDING,
    CMD_RENDER_TARGET,
    CMD_CBUFFER_INSTANCE,
    CMD_DRAW,
    CMD_DRAW_INDEXED,
    CMD_DRAW_INDIRECT,
    CMD_DRAW_INDEXED_INDIRECT,
};

struct Command {
    Command_Type type = CMD_NONE;
    
    union {
        struct { Polygon_Mode  polygon_mode; };
        struct { Topology_Mode topology_mode; };
        struct { Cull_Face cull_face; Winding_Type cull_winding; };
        struct { s32 x; s32 y; u32 w; u32 h; };
        struct { bool blend_write; };
        struct { Blend_Func blend_src; Blend_Func blend_dst; };
        struct { bool depth_write; };
        struct { Depth_Func depth_func; };
        struct { u32 stencil_mask; };
        struct { Stencil_Func stencil_func; u32 stencil_func_cmp; u32 stencil_func_mask; };
        struct { Stencil_Func stencil_op_fail; Stencil_Func stencil_op_pass; u32 stencil_op_depth_fail; };
        struct { f32 clear_r; f32 clear_g; f32 clear_b; f32 clear_a; u32 clear_bits; };
        struct { struct Shader *shader; };
        struct { struct Texture *texture; u32 texture_binding; };
        struct { struct Vertex_Descriptor *vertex_descriptor; };
        struct { struct Vertex_Binding *vertex_binding; Gpu_Handle vertex_descriptor_handle; };
        struct { struct Render_Target *render_target; };
        struct { struct Constant_Buffer *cbuffer; };
        struct { struct Constant_Buffer_Instance *cbuffer_instance; };
        
        struct {
            union { u32 vertex_count; u32 index_count; }; u32 instance_count;
            union { u32 first_vertex; u32 first_index; }; u32 first_instance;
        };

        struct {
            struct Indirect_Buffer *indirect_buffer;
            u32 indirect_offset;
            u32 indirect_count;
            u32 indirect_stride;
        };
    };
};

struct Command_Buffer {
    Command *commands = null;
    u32 count    = 0;
    u32 capacity = 0;
};

Command_Buffer make_command_buffer (u32 capacity, Allocator alc);
void           flush               (Command_Buffer *buf);

void add_cmd               (Command_Buffer *buf, const Command &cmd);
void set_polygon           (Command_Buffer *buf, Polygon_Mode mode);
void set_topology          (Command_Buffer *buf, Topology_Mode mode);
void set_viewport          (Command_Buffer *buf, s32 x, s32 y, u32 w, u32 h);
void set_scissor           (Command_Buffer *buf, s32 x, s32 y, u32 w, u32 h);
void set_cull              (Command_Buffer *buf, Cull_Face face, Winding_Type winding);
void set_blend_write       (Command_Buffer *buf, bool write);
void set_blend_func        (Command_Buffer *buf, Blend_Func src, Blend_Func dst);
void set_depth_write       (Command_Buffer *buf, bool write);
void set_depth_func        (Command_Buffer *buf, Depth_Func func);
void set_stencil_mask      (Command_Buffer *buf, u32 mask);
void set_stencil_func      (Command_Buffer *buf, Stencil_Func func, u32 cmp, u32 mask);
void set_stencil_op        (Command_Buffer *buf, Stencil_Func fail, Stencil_Func pass, Stencil_Func depth_fail);
void clear                 (Command_Buffer *buf, f32 r, f32 g, f32 b, f32 a, u32 bits);
void set_shader            (Command_Buffer *buf, Shader *shader);
void set_texture           (Command_Buffer *buf, u32 binding, Texture *texture);
void set_vertex_descriptor (Command_Buffer *buf, Vertex_Descriptor *descriptor);
void set_vertex_binding    (Command_Buffer *buf, Vertex_Binding *binding, Gpu_Handle descriptor_handle);
void set_render_target     (Command_Buffer *buf, Render_Target *target);
void set_cbuffer_instance  (Command_Buffer *buf, Constant_Buffer_Instance *instance);
void draw                  (Command_Buffer *buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void draw_indexed          (Command_Buffer *buf, u32 index_count, u32 instance_count, u32 first_index, u32 first_instance);
void draw_indirect         (Command_Buffer *buf, Indirect_Buffer *indirect, u32 offset, u32 count, u32 stride);
void draw_indirect_indexed (Command_Buffer *buf, Indirect_Buffer *indirect, u32 offset, u32 count, u32 stride);
