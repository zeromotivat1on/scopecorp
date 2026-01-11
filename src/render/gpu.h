#pragma once

enum Gpu_Polygon_Mode : u8 {
    GPU_POLYGON_NONE,
    GPU_POLYGON_FILL,
    GPU_POLYGON_LINE,
    GPU_POLYGON_POINT,
};

enum Gpu_Topology_Mode : u8 {
    GPU_TOPOLOGY_NONE,
    GPU_TOPOLOGY_LINES,
    GPU_TOPOLOGY_TRIANGLES,
    GPU_TOPOLOGY_TRIANGLE_STRIP,
};

enum Gpu_Cull_Face : u8 {
    GPU_CULL_FACE_NONE,
    GPU_CULL_FACE_FRONT,
    GPU_CULL_FACE_BACK,
    GPU_CULL_FACE_FRONT_AND_BACK,
};

enum Gpu_Winding_Type : u8 {
    GPU_WINDING_NONE,
    GPU_WINDING_CLOCKWISE,
    GPU_WINDING_COUNTER_CLOCKWISE,
};

enum Gpu_Blend_Function : u8 {
    GPU_BLEND_FUNCTION_NONE,
    GPU_BLEND_FUNCTION_SRC_ALPHA,
    GPU_BLEND_FUNCTION_ONE_MINUS_SRC_ALPHA,
};

enum Gpu_Depth_Function : u8 {
    GPU_DEPTH_FUNCTION_NONE,
    GPU_DEPTH_FUNCTION_LESS,
    GPU_DEPTH_FUNCTION_GREATER,
    GPU_DEPTH_FUNCTION_EQUAL,
    GPU_DEPTH_FUNCTION_NOT_EQUAL,
    GPU_DEPTH_FUNCTION_LESS_EQUAL,
    GPU_DEPTH_FUNCTION_GREATER_EQUAL,
};

enum Gpu_Stencil_Function : u8 {
    GPU_STENCIL_FUNCTION_NONE,
    GPU_STENCIL_FUNCTION_ALWAYS,
    GPU_STENCIL_FUNCTION_KEEP,
    GPU_STENCIL_FUNCTION_REPLACE,
    GPU_STENCIL_FUNCTION_NOT_EQUAL,
};

enum Gpu_Clear_Bits : u32 {
    GPU_CLEAR_COLOR_BIT   = 0x1,
    GPU_CLEAR_DEPTH_BIT   = 0x2,
    GPU_CLEAR_STENCIL_BIT = 0x4,

    GPU_CLEAR_COLOR_AND_DEPTH_BITS = GPU_CLEAR_COLOR_BIT | GPU_CLEAR_DEPTH_BIT,
    GPU_CLEAR_ALL_BITS = GPU_CLEAR_COLOR_BIT | GPU_CLEAR_DEPTH_BIT | GPU_CLEAR_STENCIL_BIT,
};

enum Gpu_Buffer_Type : u8 {
    GPU_BUFFER_TYPE_NONE,
    GPU_BUFFER_TYPE_STAGING_CACHED,   // read from gpu 
    GPU_BUFFER_TYPE_STAGING_UNCACHED, // write to gpu
};

enum Gpu_Sync_Result : u8 {
    GPU_SYNC_RESULT_NONE,
    GPU_SYNC_RESULT_ALREADY_SIGNALED,
    GPU_SYNC_RESULT_TIMEOUT_EXPIRED,
    GPU_SYNC_RESULT_SIGNALED,
    GPU_SYNC_RESULT_WAIT_FAILED,
};

enum Gpu_Image_Type : u8 {
    GPU_IMAGE_TYPE_NONE,
    GPU_IMAGE_TYPE_1D,
    GPU_IMAGE_TYPE_2D,
    GPU_IMAGE_TYPE_3D,
    GPU_IMAGE_TYPE_CUBEMAP,
    GPU_IMAGE_TYPE_1D_ARRAY,
    GPU_IMAGE_TYPE_2D_ARRAY,
    GPU_IMAGE_TYPE_CUBEMAP_ARRAY,
};

enum Gpu_Image_Format : u8 {
    GPU_IMAGE_FORMAT_NONE,
    GPU_IMAGE_FORMAT_RED_8,
    GPU_IMAGE_FORMAT_RED_32I,
    GPU_IMAGE_FORMAT_RED_32U,
    GPU_IMAGE_FORMAT_RGB_8,
    GPU_IMAGE_FORMAT_RGBA_8,
    GPU_IMAGE_FORMAT_DEPTH_24_STENCIL_8,
};

enum Gpu_Sampler_Wrap : u8 {
    GPU_SAMPLER_WRAP_NONE,
    GPU_SAMPLER_WRAP_REPEAT,
    GPU_SAMPLER_WRAP_CLAMP_TO_EDGE,
    GPU_SAMPLER_WRAP_CLAMP_TO_BORDER,
    GPU_SAMPLER_WRAP_MIRROR_REPEAT,
    GPU_SAMPLER_WRAP_MIRROR_CLAMP_TO_EDGE,
};

enum Gpu_Sampler_Filter : u8 {
    GPU_SAMPLER_FILTER_NONE,
    // Magnifying and minifying filters.
    GPU_SAMPLER_FILTER_NEAREST,
    GPU_SAMPLER_FILTER_LINEAR,
    // Minifying filter only.
    GPU_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST,
    GPU_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST,
    GPU_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR,
    GPU_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR,
};

// Appliable for depth-stencil textures.
enum Gpu_Sampler_Compare_Mode : u8 {
    GPU_SAMPLER_COMPARE_MODE_NONE,
    GPU_SAMPLER_COMPARE_MODE_REF_TO_IMAGE,
};

// Appliable for depth-stencil textures.
enum Gpu_Sampler_Compare_Function : u8 {
    GPU_SAMPLER_COMPARE_FUNCTION_NONE,
    GPU_SAMPLER_COMPARE_FUNCTION_EQUAL,
    GPU_SAMPLER_COMPARE_FUNCTION_NOT_EQUAL,
    GPU_SAMPLER_COMPARE_FUNCTION_LESS,
    GPU_SAMPLER_COMPARE_FUNCTION_GREATER,
    GPU_SAMPLER_COMPARE_FUNCTION_LESS_EQUAL,
    GPU_SAMPLER_COMPARE_FUNCTION_GREATER_EQUAL,
    GPU_SAMPLER_COMPARE_FUNCTION_ALWAYS,
    GPU_SAMPLER_COMPARE_FUNCTION_NEVER,
};

enum Gpu_Vertex_Attribute_Type : u8 {
    GPU_VERTEX_ATTRIBUTE_TYPE_NONE,
    GPU_VERTEX_ATTRIBUTE_TYPE_S32,
    GPU_VERTEX_ATTRIBUTE_TYPE_U32,
    GPU_VERTEX_ATTRIBUTE_TYPE_F32,
    GPU_VERTEX_ATTRIBUTE_TYPE_V2,
    GPU_VERTEX_ATTRIBUTE_TYPE_V3,
    GPU_VERTEX_ATTRIBUTE_TYPE_V4,
};

enum Gpu_Vertex_Input_Rate : u8 {
    GPU_VERTEX_INPUT_RATE_NONE,
    GPU_VERTEX_INPUT_RATE_VERTEX,
    GPU_VERTEX_INPUT_RATE_INSTANCE,
};

enum Gpu_Shader_Stage_Type : u8 {
    GPU_SHADER_STAGE_TYPE_NONE,
    GPU_SHADER_STAGE_TYPE_VERTEX,
    GPU_SHADER_STAGE_TYPE_TESS_CONTROL,
    GPU_SHADER_STAGE_TYPE_TESS_EVALUATION,
    GPU_SHADER_STAGE_TYPE_GEOMETRY,
    GPU_SHADER_STAGE_TYPE_PIXEL,
    GPU_SHADER_STAGE_TYPE_COMPUTE,
};

enum Gpu_Descriptor_Binding_Type : u8 {
    GPU_DESCRIPTOR_BINDING_TYPE_NONE,
    GPU_DESCRIPTOR_BINDING_TYPE_SAMPLER,
    GPU_DESCRIPTOR_BINDING_TYPE_SAMPLED_IMAGE,
    GPU_DESCRIPTOR_BINDING_TYPE_STORAGE_IMAGE,
    GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER,
    GPU_DESCRIPTOR_BINDING_TYPE_STORAGE_BUFFER,
};

enum Gpu_Resource_Type : u8 {
    GPU_RESOURCE_TYPE_NONE,
    GPU_TEXTURE_2D,
    GPU_TEXTURE_2D_ARRAY,
    GPU_CONSTANT_BUFFER,
    GPU_MUTABLE_BUFFER,
};

enum Gpu_Command_Type : u8 {
    GPU_CMD_NONE,
    
    // State commands change the current render state (settings, bindings etc.)
    GPU_CMD_POLYGON,
    GPU_CMD_VIEWPORT,
    GPU_CMD_SCISSOR,
    GPU_CMD_SCISSOR_TEST,
    GPU_CMD_CULL_FACE,
    GPU_CMD_WINDING,
    GPU_CMD_BLEND_TEST,
    GPU_CMD_BLEND_FUNC,
    GPU_CMD_DEPTH_TEST,
    GPU_CMD_DEPTH_WRITE,
    GPU_CMD_DEPTH_FUNC,
    GPU_CMD_STENCIL_MASK,
    GPU_CMD_STENCIL_FUNC,
    GPU_CMD_STENCIL_OP,
    GPU_CMD_CLEAR,
    GPU_CMD_SHADER,
    GPU_CMD_IMAGE_VIEW,
    GPU_CMD_SAMPLER,
    //GPU_CMD_TEXTURE,
    GPU_CMD_VERTEX_INPUT,
    GPU_CMD_VERTEX_BINDING,
    GPU_CMD_VERTEX_BUFFER,
    GPU_CMD_INDEX_BUFFER,
    GPU_CMD_FRAMEBUFFER,
    GPU_CMD_CBUFFER_INSTANCE,

    // Draw commands issue an actual draw call.
    GPU_CMD_DRAW,
    GPU_CMD_DRAW_INDEXED,
    GPU_CMD_DRAW_INDIRECT,
    GPU_CMD_DRAW_INDEXED_INDIRECT,

    GPU_CMD_COUNT
};

extern const u64 GPU_WAIT_INFINITE;

void gpu_memory_barrier(); // @Cleanup

struct Gpu_Resource {
    Gpu_Resource_Type type = {};
    u16 binding = 0;
    String name;
};

Gpu_Resource make_gpu_resource (String name, Gpu_Resource_Type type, u16 binding);

struct Gpu_Buffer {
    Gpu_Buffer_Type type;
    Handle          handle;
    u64             size;
    u64             used;
    void           *mapped_data;
};

struct Gpu_Allocation {
    u32   buffer; // owner of this memory
    u64   offset; // offset in owner buffer
    u64   size;
    u64   used;
    void *mapped_data; // points to mapped gpu memory if owner buffer is cpu visible
};

struct Gpu_Allocator {
    u32 buffer;
    u64 used;
};

Handle          gpu_fence_sync   ();
Gpu_Sync_Result wait_client_sync (Handle sync, u64 timeout);
void            wait_gpu_sync    (Handle sync);
void            delete_gpu_sync  (Handle sync);

struct Gpu_Image {
    Handle           handle;
    Gpu_Image_Type   type;
    Gpu_Image_Format format;
    u32              mipmap_count;
    u32              width;
    u32              height;
    u32              depth;
};

struct Gpu_Image_View {
    Handle           handle;
    u32              image;
    Gpu_Image_Type   type;
    Gpu_Image_Format format;
    u32              mipmap_min;
    u32              mipmap_count;
    u32              depth_min;
    u32              depth_size;
};

struct Gpu_Sampler {
    Handle                       handle;
    Gpu_Sampler_Filter           filter_min;
    Gpu_Sampler_Filter           filter_mag;
    Gpu_Sampler_Wrap             wrap_u;
    Gpu_Sampler_Wrap             wrap_v;
    Gpu_Sampler_Wrap             wrap_w;
    Gpu_Sampler_Compare_Mode     compare_mode;
    Gpu_Sampler_Compare_Function compare_function;
    f32                          lod_min;
    f32                          lod_max;
    Color4f                      color_border;
};

struct Gpu_Shader {
    Handle                handle;
    Gpu_Shader_Stage_Type stage;
};

// @Cleanup: gpu framebuffer is not that flexible as it should be.
struct Gpu_Framebuffer {
    Handle handle;
    u32   *color_attachments;
    u32    color_attachment_count;
    u32    depth_attachment;
};

struct Gpu_Indirect_Draw_Command {
    u32 vertex_count;
    u32 instance_count;
    u32 first_vertex;
    u32 first_instance;
};

struct Gpu_Indirect_Draw_Indexed_Command {
    u32 index_count;
    u32 instance_count;
    u32 first_index;
    s32 vertex_offset;
    u32 first_instance;
};

struct Gpu_Command {
    Gpu_Command_Type type;

    union {
        Gpu_Polygon_Mode   polygon;
        Gpu_Cull_Face      cull_face;
        Gpu_Winding_Type   winding;
        Gpu_Depth_Function depth_func;
        bool               depth_test;
        bool               depth_write;
        bool               blend_test;
        bool               scissor_test;
        u32                stencil_global_mask;

        // @Cleanup
        Handle resource_handle;
        
        struct { s32 x; s32 y; u32 w; u32 h; };
        struct { u32 bind_resource; u32 bind_index; u32 bind_stride; u64 bind_offset; u64 bind_size; };
        struct { Color4f clear_color; u32 clear_bits; };
        struct { Gpu_Blend_Function blend_src; Gpu_Blend_Function blend_dst; };
        struct { Gpu_Stencil_Function stencil_test; u32 stencil_ref; u32 stencil_mask; };
        struct { Gpu_Stencil_Function stencil_fail; Gpu_Stencil_Function stencil_pass; Gpu_Stencil_Function stencil_depth_fail; };

        struct {
            Gpu_Topology_Mode topology;
            union {
                struct { u32 draw_count; u32 instance_count; u32 first_draw; u32 first_instance; };
                struct { u64 indirect_offset; u32 indirect_count; u32 indirect_stride; void *indirect_data; };
            };
        };
    };
};

struct Gpu_Command_Buffer {
    Handle       handle;
    Gpu_Command *commands;
    u32          count;
    u32          capacity;
};

struct Gpu_Vertex_Attribute {
    Gpu_Vertex_Attribute_Type type;
    u32                       index;   // this attribute index
    u32                       binding; // index of binding this attribute belongs to
    u32                       offset;  // local offset relative to other binding attributes
};

struct Gpu_Vertex_Binding {
    Gpu_Vertex_Input_Rate input_rate;
    u32                   index;
    u32                   stride;
};

struct Gpu_Vertex_Input {
    Handle                handle;
    Gpu_Vertex_Binding   *bindings;
    Gpu_Vertex_Attribute *attributes;
    u32                   binding_count;
    u32                   attribute_count;
};

struct Gpu_Descriptor_Binding {
    Gpu_Descriptor_Binding_Type type;
    u32                         index;
    u32                         count;
};

struct Gpu_Descriptor {
    Handle                  handle;
    Gpu_Descriptor_Binding *bindings;
    u32                     binding_count;
};

struct Gpu_Pipeline {
    Handle handle;
    u32   *shaders;
    u32   *descriptors;
    u32    shader_count;
    u32    descriptor_count;
    u32    vertex_input;
};

// @Todo: use gpu shaders and pipelines

struct Gpu {
    Array <Gpu_Buffer>         buffers;
    Array <Gpu_Image>          images;
    Array <Gpu_Image_View>     image_views;
    Array <Gpu_Sampler>        samplers;
    Array <Gpu_Shader>         shaders;
    Array <Gpu_Framebuffer>    framebuffers;
    Array <Gpu_Command_Buffer> cmd_buffers;
    Array <Gpu_Vertex_Input>   vertex_inputs;
    Array <Gpu_Descriptor>     descriptors;
    Array <Gpu_Pipeline>       pipelines;
    Array <u32>                free_buffers;
    Array <u32>                free_images;
    Array <u32>                free_image_views;
    Array <u32>                free_samplers;
    Array <u32>                free_shaders;
    Array <u32>                free_framebuffers;
    Array <u32>                free_cmd_buffers;
    Array <u32>                free_vertex_inputs;
    Array <u32>                free_descriptors;
    Array <u32>                free_pipelines;
    String                     vendor;
    String                     renderer;
    String                     backend_version;
    String                     shader_language_version;
    u32                        default_image;
    u32                        default_image_view;
    u32                        sampler_default_color;
    u32                        sampler_default_depth_stencil;
    u32                        default_framebuffer;
    u32                        vertex_input_entity;
    u32                        descriptor_global;
    u32                        descriptor_entity;
};

void                gpu_init_backend                    ();
void                gpu_init_frontend                   ();
u32                 gpu_uniform_buffer_max_size         ();
u32                 gpu_uniform_buffer_offset_alignment ();
u32                 gpu_image_max_size                  ();
u32                 gpu_vertex_attribute_max_count      ();
u32                 gpu_new_buffer                      (Gpu_Buffer_Type type, u64 size);
u32                 gpu_new_image                       (Gpu_Image_Type type, Gpu_Image_Format format, u32 mipmap_count, 
                                                         u32 width, u32 height, u32 depth, Buffer base_mipmap_contents);
u32                 gpu_new_image_view                  (u32 image, Gpu_Image_Type type, Gpu_Image_Format format,
                                                         u32 mipmap_min, u32 mipmap_count, u32 depth_min, u32 depth_size);
u32                 gpu_new_sampler                     (Gpu_Sampler_Filter filter_min, Gpu_Sampler_Filter filter_mag,
                                                         Gpu_Sampler_Wrap wrap_u, Gpu_Sampler_Wrap wrap_v, Gpu_Sampler_Wrap wrap_w,
                                                         Gpu_Sampler_Compare_Mode compare_mode, Gpu_Sampler_Compare_Function compare_function,
                                                         f32 lod_min, f32 lod_max, Color4f color_border);
u32                 gpu_new_shader                      (Gpu_Shader_Stage_Type stage, Buffer source);
u32                 gpu_new_framebuffer                 (const Gpu_Image_Format *color_formats, u32 color_format_count, u32 color_width, u32 color_height,
                                                         Gpu_Image_Format depth_format, u32 depth_width, u32 depth_height);
u32                 gpu_new_command_buffer              (u32 capacity);
u32                 gpu_new_vertex_input                (const Gpu_Vertex_Binding *bindings, u32 count, const Gpu_Vertex_Attribute *attributes, u32 attribute_count);
u32                 gpu_new_descriptor                  (const Gpu_Descriptor_Binding *bindings, u32 count);
u32                 gpu_new_pipeline                    (u32 vertex_input, u32 *shaders, u32 shader_count, u32 *descriptors, u32 descriptor_count);
void                gpu_delete_buffer                   (u32 index);
void                gpu_delete_image                    (u32 index);
void                gpu_delete_image_view               (u32 index);
void                gpu_delete_sampler                  (u32 index);
void                gpu_delete_framebuffer              (u32 index);
void                gpu_delete_command_buffer           (u32 index);
void                gpu_delete_vertex_input             (u32 index);
void                gpu_delete_descriptor               (u32 index);
void                gpu_delete_pipeline                 (u32 index);
Gpu_Buffer         *gpu_get_buffer                      (u32 index);
Gpu_Image          *gpu_get_image                       (u32 index);
Gpu_Image_View     *gpu_get_image_view                  (u32 index);
Gpu_Sampler        *gpu_get_sampler                     (u32 index);
Gpu_Shader         *gpu_get_shader                      (u32 index);
Gpu_Framebuffer    *gpu_get_framebuffer                 (u32 index);
Gpu_Command_Buffer *gpu_get_command_buffer              (u32 index);
Gpu_Vertex_Input   *gpu_get_vertex_input                (u32 index);
Gpu_Descriptor     *gpu_get_descriptor                  (u32 index);
Gpu_Pipeline       *gpu_get_pipeline                    (u32 index);
Gpu_Allocation      gpu_alloc                           (u64 size,                Gpu_Allocator *alc);
Gpu_Allocation      gpu_alloc                           (u64 size, u64 alignment, Gpu_Allocator *alc);
void                gpu_release                         (Gpu_Allocation *memory,  Gpu_Allocator *alc);
void                gpu_append                          (Gpu_Allocation *memory, const void *data, u64 size);
void                gpu_flush_cmd_buffer                (u32 cmd_buffer);
void                gpu_add_cmds                        (u32 cmd_buffer, Gpu_Command *cmds, u32 count);
void                gpu_cmd_polygon                     (u32 cmd_buffer, Gpu_Polygon_Mode mode);
void                gpu_cmd_viewport                    (u32 cmd_buffer, s32 x, s32 y, u32 w, u32 h);
void                gpu_cmd_scissor                     (u32 cmd_buffer, s32 x, s32 y, u32 w, u32 h);
void                gpu_cmd_cull_face                   (u32 cmd_buffer, Gpu_Cull_Face face);
void                gpu_cmd_winding                     (u32 cmd_buffer, Gpu_Winding_Type winding);
void                gpu_cmd_scissor_test                (u32 cmd_buffer, bool test);
void                gpu_cmd_blend_test                  (u32 cmd_buffer, bool test);
void                gpu_cmd_blend_func                  (u32 cmd_buffer, Gpu_Blend_Function src, Gpu_Blend_Function dst);
void                gpu_cmd_depth_test                  (u32 cmd_buffer, bool test);
void                gpu_cmd_depth_write                 (u32 cmd_buffer, bool write);
void                gpu_cmd_depth_func                  (u32 cmd_buffer, Gpu_Depth_Function func);
void                gpu_cmd_stencil_mask                (u32 cmd_buffer, u32 mask);
void                gpu_cmd_stencil_func                (u32 cmd_buffer, Gpu_Stencil_Function func, u32 cmp, u32 mask);
void                gpu_cmd_stencil_op                  (u32 cmd_buffer, Gpu_Stencil_Function fail, Gpu_Stencil_Function pass, Gpu_Stencil_Function depth_fail);
void                gpu_cmd_clear                       (u32 cmd_buffer, Color4f color, u32 bits);
// @Todo: remove this overload.
void                gpu_cmd_shader                      (u32 cmd_buffer, struct Shader *shader);
//void                gpu_cmd_shader                (u32 cmd_buffer, u32 shader);
void                gpu_cmd_image_view                  (u32 cmd_buffer, u32 binding, u32 image_view);
void                gpu_cmd_sampler                     (u32 cmd_buffer, u32 binding, u32 sampler);
void                gpu_cmd_vertex_input                (u32 cmd_buffer, u32 vertex_input);
void                gpu_cmd_vertex_buffer               (u32 cmd_buffer, u32 buffer, u32 binding, u64 offset, u32 stride);
//void                gpu_cmd_vertex_binding        (u32 cmd_buffer, Vertex_Binding *binding, Handle descriptor_handle);
void                gpu_cmd_index_buffer                (u32 cmd_buffer, u32 buffer);
void                gpu_cmd_cbuffer_instance            (u32 cmd_buffer, struct Constant_Buffer_Instance *instance);
void                gpu_cmd_framebuffer                 (u32 cmd_buffer, u32 framebuffer);
void                gpu_cmd_draw                        (u32 cmd_buffer, Gpu_Topology_Mode topology, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
void                gpu_cmd_draw_indexed                (u32 cmd_buffer, Gpu_Topology_Mode topology, u32 index_count, u32 instance_count, u32 first_index, u32 first_instance);
void                gpu_cmd_draw_indirect               (u32 cmd_buffer, Gpu_Topology_Mode topology, void *data, u32 offset, u32 count, u32 stride);
void                gpu_cmd_draw_indirect_indexed       (u32 cmd_buffer, Gpu_Topology_Mode topology, void *data, u32 offset, u32 count, u32 stride);
u32                 gpu_max_mipmap_count                (u32 width, u32 height);
Gpu_Image_Format    gpu_image_format_from_channel_count (u32 channel_count);
u32                 gpu_vertex_attribute_size           (Gpu_Vertex_Attribute_Type type);
u32                 gpu_vertex_attribute_dimension      (Gpu_Vertex_Attribute_Type type);

template <typename T>
void gpu_append(Gpu_Allocation *memory, const T &t) {
    gpu_append(memory, &t, sizeof(t));
}

// Helper function to get next free index during gpu resource creation.
template <typename T, typename I>
u32 gpu_next_index(Array <T> &resources, Array <I> &free_indices) {
    const auto count = resources.count;
    const auto index = free_indices.count ? array_pop(free_indices) : ((void)array_add(resources, {}), count);
    return index;
}

inline String to_string(Gpu_Shader_Stage_Type stage) {
    static const String lut[] = {
        S("none"), S("vertex"), S("tess_control"), S("tess_evaluation"),
        S("geometry"), S("pixel"), S("compute"),
    };
    return lut[stage];
}

inline Gpu           gpu;
inline Gpu_Allocator gpu_read_allocator;
inline Gpu_Allocator gpu_write_allocator;
