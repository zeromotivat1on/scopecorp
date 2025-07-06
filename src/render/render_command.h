 #pragma once

#include "math/vector.h"

inline constexpr s32 MAX_RENDER_QUEUE_SIZE = 1024;

struct Entity;
struct World;
struct Window;

enum Clear_Flag : u8 {
    CLEAR_FLAG_COLOR   = 0x1,
    CLEAR_FLAG_DEPTH   = 0x2,
    CLEAR_FLAG_STENCIL = 0x4,
};

enum Render_Mode : u8 {
	RENDER_TRIANGLES,
	RENDER_TRIANGLE_STRIP,
	RENDER_LINES,
};

enum Render_Flag : u32 {
    RENDER_FLAG_VIEWPORT  = 0x1,
    RENDER_FLAG_SCISSOR   = 0x2,
    RENDER_FLAG_CLEAR     = 0x4,
    RENDER_FLAG_CULL_FACE = 0x8,
    RENDER_FLAG_BLEND     = 0x10,
    RENDER_FLAG_DEPTH     = 0x20,
    RENDER_FLAG_STENCIL   = 0x40,
    RENDER_FLAG_INDEXED   = 0x40000000, // render using index buffer
    RENDER_FLAG_RESET     = 0x80000000, // reset render state after draw call
};

enum Polygon_Mode : u8 {
    POLYGON_FILL,
    POLYGON_LINE,
    POLYGON_POINT,
};

enum Winding_Type : u8 {
    WINDING_CLOCKWISE,
    WINDING_COUNTER_CLOCKWISE,
};

enum Cull_Face_Type : u8 {
    CULL_FACE_BACK,
    CULL_FACE_FRONT,
};

enum Blend_Test_Function_Type : u8 {
    BLEND_SOURCE_ALPHA,
    BLEND_ONE_MINUS_SOURCE_ALPHA,
};

enum Depth_Test_Function_Type : u8 {
    DEPTH_LESS,
};

enum Depth_Test_Mask_Type : u8 {
    DEPTH_ENABLE,
    DEPTH_DISABLE,
};

enum Stencil_Test_Operation_Type : u8 {
    STENCIL_KEEP,
    STENCIL_REPLACE,
};

enum Stencil_Test_Function_Type : u8 {
    STENCIL_ALWAYS,
    STENCIL_EQUAL,
    STENCIL_NOT_EQUAL,
};

struct Viewport_Test {
    s16 x = 0;
    s16 y = 0;
    s16 width  = 0;
    s16 height = 0;
};

struct Scissor_Test {
    s16 x = 0;
    s16 y = 0;
    s16 width  = 0;
    s16 height = 0;
};

struct Clear_Test {
    vec3 color;
    u32  flags;
};

struct Cull_Face_Test {
    Cull_Face_Type type;
    Winding_Type   winding;
};

struct Blend_Test {
    Blend_Test_Function_Type source;
    Blend_Test_Function_Type destination;
};

struct Depth_Test {
    Depth_Test_Function_Type function;
    Depth_Test_Mask_Type     mask;
};

struct Stencil_Test_Operation {
    Stencil_Test_Operation_Type stencil_failed;
    Stencil_Test_Operation_Type depth_failed;
    Stencil_Test_Operation_Type both_passed;
};

struct Stencil_Test_Function {
    Stencil_Test_Function_Type type;
    u8 comparator;
    u8 mask;
};

struct Stencil_Test {
    Stencil_Test_Operation operation;
    Stencil_Test_Function  function;
    u8 mask;
};

struct Uniform;

struct Render_Command {
	u32 flags = 0;
    
	Render_Mode  render_mode  = RENDER_TRIANGLES;
	Polygon_Mode polygon_mode = POLYGON_FILL;

    Viewport_Test  viewport;
    Scissor_Test   scissor;
    Cull_Face_Test cull_face;
    Clear_Test     clear;
    Blend_Test     blend;
    Depth_Test     depth;
    Stencil_Test   stencil;

	rid rid_frame_buffer = RID_NONE;
    rid rid_vertex_array = RID_NONE;

    // @Todo: make it more generic like with mesh sids that set vertex info.
    sid sid_material = SID_NONE;

    // @Hack: handle run-time created textures that are not asset and so can't be used
    // by materials.
    // @Update: font atlas texture is passed here, as it won't work with materials.
    // Actual solution would be to prebake font atlas as actual texture and store it
    // in asset pak.
    // @Update: this is also used by frame buffer color attachments...
    rid rid_override_texture = RID_NONE;
    
    // Offset from local buffer pointer for vertex data if vertex array is provided, so the
    // final vertex offset will be as follows: vertex_array.offset + buffer_element_offset.
    // Or offset from global index buffer storage if RENDER_FLAG_INDEXED is set.
    u32 buffer_element_offset = 0;
    u32 buffer_element_count  = 0;

	u32 instance_count  = 1;
	u32 instance_offset = 0;

    // Editor-only.
    u32 eid_vertex_data_offset = 0;
};

struct Render_Queue {
	Render_Command *commands = null;
	s32 size     = 0;
	s32 capacity = 0;
};

inline Render_Queue entity_render_queue;

void init_render_queue(Render_Queue *queue, s32 capacity);
void r_enqueue(Render_Queue *queue, const Render_Command *command);
void r_flush(Render_Queue *queue);
void r_submit(const Render_Command *command);
