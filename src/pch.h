#pragma once

#include "preload.h"
#include "vector.h"
#include "matrix.h"
#include "quaternion.h"
#include "controls.h"

#define GAME_NAME    "scopecorp"
#define GAME_VERSION "0.4.0"

// typedef u64 sid; // string id
// typedef u32 eid; // entity id

#if OPEN_GL
//typedef u32 rid; // render id, used by underlying gfx api
#else
#error "Unsupported graphics api"
#endif

#define MAX_PATH_LENGTH 256

struct Color4f { f32 r, g, b, a; };
union  Color32 { u32 hex; struct { u32 a : 8; u32 b : 8; u32 g : 8; u32 r : 8; }; };

inline constexpr auto COLOR4F_BLACK  = Color4f { .r = 0, .g = 0, .b = 0, .a = 1 };
inline constexpr auto COLOR4F_WHITE  = Color4f { .r = 1, .g = 1, .b = 1, .a = 1 };
inline constexpr auto COLOR4F_RED    = Color4f { .r = 1, .g = 0, .b = 0, .a = 1 };
inline constexpr auto COLOR4F_GREEN  = Color4f { .r = 0, .g = 1, .b = 0, .a = 1 };
inline constexpr auto COLOR4F_BLUE   = Color4f { .r = 0, .g = 0, .b = 1, .a = 1 };
inline constexpr auto COLOR4F_YELLOW = Color4f { .r = 1, .g = 1, .b = 0, .a = 1 };
inline constexpr auto COLOR4F_PURPLE = Color4f { .r = 1, .g = 0, .b = 1, .a = 1 };
inline constexpr auto COLOR4F_CYAN   = Color4f { .r = 0, .g = 1, .b = 1, .a = 1 };

inline constexpr auto COLOR32_BLACK  = Color32 { .hex = 0x000000FF };
inline constexpr auto COLOR32_WHITE  = Color32 { .hex = 0xFFFFFFFF };
inline constexpr auto COLOR32_RED    = Color32 { .hex = 0xFF0000FF };
inline constexpr auto COLOR32_GREEN  = Color32 { .hex = 0x00FF00FF };
inline constexpr auto COLOR32_BLUE   = Color32 { .hex = 0x0000FFFF };
inline constexpr auto COLOR32_YELLOW = Color32 { .hex = 0xFFFF00FF };
inline constexpr auto COLOR32_PURPLE = Color32 { .hex = 0xFF00FFFF };
inline constexpr auto COLOR32_CYAN   = Color32 { .hex = 0x00FFFFFF };
inline constexpr auto COLOR32_GRAY   = Color32 { .hex = 0xAAAAAAFF };

// Just some explicit ascii characters for clarity.
#define C_BACKSPACE       8
#define C_TAB             9
#define C_NEW_LINE        10
#define C_VERTICAL_TAB    11
#define C_FORM_FEED       12
#define C_CARRIAGE_RETURN 13
#define C_ESCAPE          27
#define C_SPACE           32
#define C_GRAVE_ACCENT    96

#define _sizeref(x) sizeof(x), &x

#define DIR_CODEGEN       S("../src/codegen/")
#define DIR_SHADERS       S("data/shaders/")
#define DIR_TEXTURES      S("data/textures/")
#define DIR_MATERIALS     S("data/materials/")
#define DIR_MESHES        S("data/meshes/")
#define DIR_SOUNDS        S("data/sounds/")
#define DIR_FONTS         S("data/fonts/")
#define DIR_FLIP_BOOKS    S("data/flip_books/")
#define DIR_LEVELS        S("data/levels/")

#define PATH_CODEGEN(x)   S("../src/codegen/"  x)
#define PATH_PAK(x)       S(""                 x)
#define PATH_SHADER(x)    S("data/shaders/"    x)
#define PATH_TEXTURE(x)   S("data/textures/"   x)
#define PATH_MATERIAL(x)  S("data/materials/"  x)
#define PATH_MESH(x)      S("data/meshes/"     x)
#define PATH_SOUND(x)     S("data/sounds/"     x)
#define PATH_FONT(x)      S("data/fonts/"      x)
#define PATH_FLIP_BOOK(x) S("data/flip_books/" x)
#define PATH_LEVEL(x)     S("data/levels/"     x)

inline const auto GAME_PAK_PATH = PATH_PAK(GAME_NAME ".pak");

String get_build_type_name();

String get_file_name_no_ext (String path);
String get_extension        (String path);

enum For_Result : u8 {
    CONTINUE,
    BREAK,
};

enum Direction : u8 {
    SOUTH,
    EAST,
    WEST,
    NORTH,
    DIRECTION_COUNT
};

struct Rect {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 w = 0.0f;
    f32 h = 0.0f;
};

void sort (void *data, u32 count, u32 size, s32 (*compare)(const void *, const void *));

inline constexpr u32 MAX_DIRECT_LIGHTS = 4;
inline constexpr u32 MAX_POINT_LIGHTS  = 32;

struct Time_Info {
    u64 frame_start_counter = 0;
    u64 frame_end_counter   = 0;
    
    f32 delta_time = 0.0f;
};

inline Time_Info time_info;
bool should_quit_game = false;
bool in_main_loop = false;
u64 frame_index = 0;
u64 highest_water_mark = 0;

inline f32 get_delta_time () { return time_info.delta_time; }

enum Camera_Mode : u8 {
	PERSPECTIVE,
	ORTHOGRAPHIC
};

struct Camera {
	Camera_Mode mode;

    Vector3 position;
	Vector3 look_at_position;
	Vector3 up_vector;

	f32 yaw; // horizontal rotation around y-axis
	f32 pitch; // vertical rotation around x-axis

	f32	fov;
	f32	aspect;

	f32	near_plane;
	f32	far_plane;
	f32	left;
	f32	right;
	f32	bottom;
	f32	top;

    Matrix4 view;
    Matrix4 proj;
    Matrix4 view_proj;
    Matrix4 inv_view;
    Matrix4 inv_proj;
};

struct Viewport;

void update_matrices    (Camera &c);
void on_viewport_resize (Camera &c, const Viewport &vp);

Vector2 world_to_screen (Vector3 position, const Camera &camera, const Rect &rect);
Vector3 screen_to_world (Vector2 position, const Camera &camera, const Rect &rect);

struct Entity_Manager;

struct Campaign_State {
    String campaign_name;
    Entity_Manager *entity_manager = null;
};

Campaign_State *get_campaign_state ();

struct Level_Set {
    String name;
    Campaign_State *campaign_state = null;
};

inline Level_Set *current_level_set = null;

Level_Set *get_level_set ();
void switch_campaign (Level_Set *set);

void init_level_general    (bool reset);
void init_level_editor_hub ();

struct Window;
inline Window *main_window = null;
inline Window *get_window () { Assert(main_window); return main_window; }

// @Cleanup
struct Gpu_Picking_Data { f32 depth; Pid eid; };
inline Gpu_Picking_Data *gpu_picking_data = null;
inline u64 gpu_picking_data_offset;
inline u32 gpu_picking_buffer_binding = 5;
