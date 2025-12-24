#pragma once

#include "preload.h"
#include "vector.h"
#include "matrix.h"
#include "quaternion.h"

#define GAME_NAME    "scopecorp"
#define GAME_VERSION "0.4.0"

// typedef u64 sid; // string id
// typedef u32 eid; // entity id

#if OPEN_GL
//typedef u32 rid; // render id, used by underlying gfx api
#else
#error "Unsupported graphics api"
#endif

// inline constexpr sid SID_NONE = 0;
// inline constexpr eid EID_NONE = 0;
// inline constexpr eid EID_MAX  = U32_MAX;
//inline constexpr rid RID_NONE = 0;

#define INDEX_NONE -1
#define MAX_PATH_LENGTH 256

#define rgba_pack(r, g, b, a) (u32)((r) << 24  | (g) << 16  | (b) << 8 | (a) << 0)

#define rgba_set_r(c, r) (((c) & 0x00FFFFFF) | ((u32)(r) << 24))
#define rgba_set_g(c, g) (((c) & 0xFF00FFFF) | ((u32)(g) << 16))
#define rgba_set_b(c, b) (((c) & 0xFFFF00FF) | ((u32)(b) << 8))
#define rgba_set_a(c, a) (((c) & 0xFFFFFF00) | ((u32)(a) << 0))

#define rgba_get_r(c) (((c) >> 24) & 0xFF)
#define rgba_get_g(c) (((c) >> 16) & 0xFF)
#define rgba_get_b(c) (((c) >> 8)  & 0xFF)
#define rgba_get_a(c) (((c) >> 0)  & 0xFF)

#define rgba_white  rgba_pack(255, 255, 255, 255)
#define rgba_black  rgba_pack(  0,   0,   0, 255)
#define rgba_red    rgba_pack(255,   0,   0, 255)
#define rgba_green  rgba_pack(  0, 255,   0, 255)
#define rgba_blue   rgba_pack(  0,   0, 255, 255)
#define rgba_yellow rgba_pack(255, 255,   0, 255)
#define rgba_purple rgba_pack(255,   0, 255, 255)
#define rgba_cyan   rgba_pack(  0, 255, 255, 255)

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

#if DEBUG
inline const char* Build_type_name = "DEBUG";
#elif RELEASE
inline const char* Build_type_name = "RELEASE";
#else
#error "Unknown build type"
#endif

#define _sizeref(x) sizeof(x), &x

#define PATH_PAK(x)       S(""                 x)
#define PATH_SHADER(x)    S("data/shaders/"    x)
#define PATH_TEXTURE(x)   S("data/textures/"   x)
#define PATH_MATERIAL(x)  S("data/materials/"  x)
#define PATH_MESH(x)      S("data/meshes/"     x)
#define PATH_SOUND(x)     S("data/sounds/"     x)
#define PATH_FONT(x)      S("data/fonts/"      x)
#define PATH_FLIP_BOOK(x) S("data/flip_books/" x)
#define PATH_LEVEL(x)     S("data/levels/"     x)

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

enum Program_Mode : u8 {
    MODE_GAME,
    MODE_EDITOR,

    PROGRAM_MODE_COUNT
};

inline Program_Mode program_mode = MODE_EDITOR;

struct Time_Info {
    u64 frame_start_counter = 0;
    u64 frame_end_counter   = 0;
    
    f32 delta_time = 0.0f;
};

inline Time_Info time_info;
bool should_quit_game = false;
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
Gpu_Picking_Data *gpu_picking_data = null;

#include "render_api.h"
