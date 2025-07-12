#pragma once

#include "font.h"
#include "math/vector.h"

inline constexpr u8  MAX_UI_FONT_ATLASES = 32;
inline constexpr u16 MAX_UI_DRAW_QUEUE_SIZE = 1024;
inline constexpr u16 MAX_UI_TEXT_DRAW_BUFFERS = 8;
inline constexpr u16 MAX_UI_TEXT_DRAW_BUFFER_CHARS = 4096;
inline constexpr u16 MAX_UI_QUAD_DRAW_BUFFER_QUADS = 512;

inline constexpr s32 UI_DEFAULT_FONT_ATLAS_INDEX       = 0;
inline constexpr s32 UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX = 1;
inline constexpr s32 UI_PROFILER_FONT_ATLAS_INDEX      = 2;
inline constexpr s32 UI_SCREEN_REPORT_FONT_ATLAS_INDEX = 3;

inline constexpr u32 UI_INPUT_BUFFER_SIZE_F32 = 16;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_S8  = 4;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_S16 = 8;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_S32 = 16;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_S64 = 16;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_U8  = 4;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_U16 = 8;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_U32 = 16;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_U64 = 16;
inline constexpr u32 UI_INPUT_BUFFER_SIZE_SID = 64;

struct mat4;
struct Font_Atlas;

enum UI_Flags : u8 {
    UI_FLAG_HOT       = 0x1,
    UI_FLAG_UNHOT     = 0x2,

    UI_FLAG_ACTIVATED = 0x4,
    UI_FLAG_LOST      = 0x8,
    UI_FLAG_FINISHED  = 0x10,
    
    UI_FLAG_CHANGED   = 0x20,
};

struct uiid {
    u16 owner;
    u16 item;
    u16 index;
};

inline constexpr uiid UIID_NONE = { 0, 0, 0 };

inline bool operator==(const uiid &a, const uiid &b) {
    return a.owner == b.owner && a.item == b.item && a.index == b.index;
}

inline bool operator!=(const uiid &a, const uiid &b) {
    return !(a == b);
}

struct UI_Color {
    u32 cold   = 0;
    u32 hot    = 0;
    u32 active = 0;
};

struct UI_Button_Style {
    vec3 pos_text = vec3_zero;
    vec2 padding  = vec2_zero;
    UI_Color color_text;
    UI_Color color_quad;
    s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX;
};

struct UI_Input_Style {
    vec3 pos_text = vec3_zero;
    vec2 padding  = vec2_zero;
    UI_Color color_text;
    UI_Color color_quad;
    UI_Color color_cursor;
    s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX;
};

struct UI_Combo_Style {
    vec3 pos_text = vec3_zero;
    vec2 padding  = vec2_zero;
    UI_Color color_text;
    UI_Color color_quad;
    s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX;
};

enum UI_Draw_Type : u8 {
    UI_DRAW_TEXT,
    UI_DRAW_QUAD,
};

struct UI_Draw_Command {
    UI_Draw_Type type;
    f32 z = 0.0f;
    s32 instance_count = 0;
    s32 atlas_index = 0; // for text draw command
};

struct UI_Text_Draw_Buffer {
    f32  *positions  = null;
    u32  *colors     = null;
    u32  *charmap    = null;
    mat4 *transforms = null;

    u16 char_count = 0;

    rid rid_vertex_array = RID_NONE;
    sid sid_material = SID_NONE;
};

struct UI_Quad_Draw_Buffer {
    vec2 *positions = null;
    u32  *colors    = null;

    u16 quad_count = 0;

    rid rid_vertex_array = RID_NONE;
    sid sid_material = SID_NONE;
};

struct UI {
    Font_Atlas font_atlases[MAX_UI_FONT_ATLASES];
    UI_Draw_Command draw_queue[MAX_UI_DRAW_QUEUE_SIZE];

    u8  font_atlas_count = 0;
    u16 draw_queue_size  = 0;

    UI_Text_Draw_Buffer text_draw_buffer;
    UI_Quad_Draw_Buffer quad_draw_buffer;

    uiid id_hot;
    uiid id_active;
};

inline UI ui;
inline Font_Info ui_default_font;

void ui_init();
void ui_flush();

u8 ui_button(uiid id, const char *text, const UI_Button_Style &style);
u8 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style);

u8 ui_input_f32(uiid id, f32 *v, const UI_Input_Style &style);

u8 ui_input_s8 (uiid id, s8  *v, const UI_Input_Style &style);
u8 ui_input_s16(uiid id, s16 *v, const UI_Input_Style &style);
u8 ui_input_s32(uiid id, s32 *v, const UI_Input_Style &style);
u8 ui_input_s64(uiid id, s64 *v, const UI_Input_Style &style);

u8 ui_input_u8 (uiid id, u8  *v, const UI_Input_Style &style);
u8 ui_input_u16(uiid id, u16 *v, const UI_Input_Style &style);
u8 ui_input_u32(uiid id, u32 *v, const UI_Input_Style &style);
u8 ui_input_u64(uiid id, u64 *v, const UI_Input_Style &style);

u8 ui_input_sid(uiid id, sid *v, const UI_Input_Style &style);

u8 ui_combo(uiid, u32 *selected_index, const char **options, u32 option_count, const UI_Combo_Style &style);

void ui_text(const char *text, vec3 pos, u32 color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_text(const char *text, u32 count, vec3 pos, u32 color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);

void ui_text_with_shadow(const char *text, vec3 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_text_with_shadow(const char *text, u32 count, vec3 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);

void ui_quad(vec3 p0, vec3 p1, u32 color);
