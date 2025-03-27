 #pragma once

inline constexpr s32 MAX_RENDER_QUEUE_SIZE = 1024;
inline constexpr s32 MAX_RENDER_COMMAND_UNIFORMS = 16;

struct Entity;
struct World;
struct Window;
struct vec3;

enum Clear_Screen_Flag {
    CLEAR_FLAG_COLOR,
    CLEAR_FLAG_DEPTH,
    CLEAR_FLAG_STENCIL,
};

enum Render_Mode {
	RENDER_TRIANGLES,
	RENDER_TRIANGLE_STRIP,
	RENDER_LINES,
};

enum Render_Flag : u32 {
    RENDER_FLAG_SCISSOR_TEST   = 0x1,
    RENDER_FLAG_CULL_FACE_TEST = 0x2,
    RENDER_FLAG_BLEND_TEST     = 0x4,
    RENDER_FLAG_DEPTH_TEST     = 0x8,
    RENDER_FLAG_STENCIL_TEST   = 0x10,
    RENDER_FLAG_RESET          = 0x80000000, // reset render state after draw call
};

enum Polygon_Mode {
    POLYGON_FILL,
    POLYGON_LINE,
    POLYGON_POINT,
};

enum Winding_Type {
    WINDING_CLOCKWISE,
    WINDING_COUNTER_CLOCKWISE,
};

enum Cull_Face_Type {
    CULL_FACE_BACK,
    CULL_FACE_FRONT,
};

enum Blend_Test_Function_Type {
    BLEND_TEST_SOURCE_ALPHA,
    BLEND_TEST_ONE_MINUS_SOURCE_ALPHA,
};

enum Depth_Test_Function_Type {
    DEPTH_TEST_LESS,
};

enum Depth_Test_Mask_Type {
    DEPTH_TEST_ENABLE,
    DEPTH_TEST_DISABLE,
};

enum Stencil_Test_Operation_Type {
    STENCIL_TEST_KEEP,
    STENCIL_TEST_REPLACE,
};

enum Stencil_Test_Function_Type {
    STENCIL_TEST_ALWAYS,
    STENCIL_TEST_EQUAL,
    STENCIL_TEST_NOT_EQUAL,
};

struct Scissor_Test {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
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

struct Render_Command {
	u32 flags = 0;
    
	Render_Mode  render_mode  = RENDER_TRIANGLES;
	Polygon_Mode polygon_mode = POLYGON_FILL;

    Scissor_Test   scissor_test;
    Cull_Face_Test cull_face_test;
    Blend_Test     blend_test;
    Depth_Test     depth_test;
    Stencil_Test   stencil_test;

	s32 frame_buffer_index = INVALID_INDEX;
	s32 vertex_array_index = INVALID_INDEX;
	s32 index_buffer_index = INVALID_INDEX;

    s32 shader_index  = INVALID_INDEX;
	s32 texture_index = INVALID_INDEX;
    
    s32 uniform_count = 0;
    s32 uniform_indices      [MAX_RENDER_COMMAND_UNIFORMS];
    s32 uniform_value_offsets[MAX_RENDER_COMMAND_UNIFORMS];

    s32 buffer_element_count  = 0;
    s32 buffer_element_offset = 0;
    
	s32 instance_count = 1;
};

struct Render_Queue {
	Render_Command *commands = null;
	s32 size     = 0;
	s32 capacity = 0;
};

inline Render_Queue entity_render_queue;

// @Cleanup: move these to separate header.
void init_render(Window *window);
void swap_buffers(Window *window);
void set_vsync(bool enable);
void clear_screen(vec3 color, u32 flags);

void init_render_queue(Render_Queue *queue, s32 capacity);
void enqueue(Render_Queue *queue, const Render_Command *command);
void flush(Render_Queue *queue);
void submit(const Render_Command *command);
