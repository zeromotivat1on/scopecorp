#pragma once

#include "font.h"

inline constexpr f32 UI_MAX_Z = 1000.0f;

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

// Bits that are returned from immediate ui elements.
enum UI_Immediate_Bits : u16 {
    UI_HOT_BIT       = 0x1,  // cursor hover
    UI_UNHOT_BIT     = 0x2,  // cursor unhover
    UI_ACTIVATED_BIT = 0x4,  // received focus (button click or input select)
    UI_LOST_BIT      = 0x8,  // action was completed as NOT planned
    UI_FINISHED_BIT  = 0x10, // action was completed as planned
    UI_CHANGED_BIT   = 0x20, // some data was changed (e.g: input text or combo option)
};

struct uiid {
    u16 owner;
    u16 item;
    u16 index;
};

inline constexpr uiid UIID_NONE = { 0, 0, 0 };

bool operator==(const uiid &a, const uiid &b) { return a.owner == b.owner && a.item == b.item && a.index == b.index; }
bool operator!=(const uiid &a, const uiid &b) { return !(a == b); }

struct UI_Color {
    Color32 cold   = 0;
    Color32 hot    = 0;
    Color32 active = 0;
};

struct UI_Style {
    Font_Atlas *font_atlas = global_font_atlases.main_small;

    f32 z = 0.0f;

    Vector2 pos     = Vector2_zero;
    Vector2 padding = Vector2_zero;
    Vector2 margin  = Vector2_zero;

    UI_Color back_color;
    UI_Color front_color;
};

struct UI_Button_Style : UI_Style {
};

struct UI_Input_Style : UI_Style {
    UI_Color cursor_color;
};

struct UI_Combo_Style : UI_Style {
};

struct Material;

struct UI_Context {    
    struct Line_Render {
        static constexpr u16 MAX_LINES = 4096;
    
        Vector2  *positions;
        Color32  *colors;
        u64       positions_offset;
        u64       colors_offset;
        u32       line_count;
        u32       vertex_input;
        Atom      material;
    };

    struct Quad_Render {
        static constexpr u32 MAX_QUADS = 128;
    
        Vector2  *positions;
        Color32  *colors;
        u64       positions_offset;
        u64       colors_offset;
        u32       quad_count;
        u32       vertex_input;
        Atom      material;
    };

    struct Text_Render {
        static constexpr u16 MAX_CHARS = 4096;

        f32      *positions;
        Vector4  *uv_rects;
        Color32  *colors;
        u32      *charmap;
        Matrix4  *transforms;
        u64       positions_offset;
        u64       uv_rects_offset;
        u64       colors_offset;
        u64       charmap_offset;
        u64       transforms_offset;
        u32       char_count;
        u32       vertex_input;
        Atom      material;
    };
    
    uiid                 id_hot;
    uiid                 id_active;
    Table <uiid, char *> input_table;
    String               input_buffer;
    Line_Render          line_render;
    Text_Render          text_render;
    Quad_Render          quad_render;
};

inline UI_Context ui;

void init_ui             ();
u16  ui_button           (uiid id, String text, UI_Button_Style style);
u16  ui_input_text       (uiid id, char *text, u32 size, UI_Input_Style style);
u16  ui_input_f32        (uiid id, f32 *v, UI_Input_Style style);
u16  ui_input_s8         (uiid id, s8  *v, UI_Input_Style style);
u16  ui_input_s16        (uiid id, s16 *v, UI_Input_Style style);
u16  ui_input_s32        (uiid id, s32 *v, UI_Input_Style style);
u16  ui_input_s64        (uiid id, s64 *v, UI_Input_Style style);
u16  ui_input_u8         (uiid id, u8  *v, UI_Input_Style style);
u16  ui_input_u16        (uiid id, u16 *v, UI_Input_Style style);
u16  ui_input_u32        (uiid id, u32 *v, UI_Input_Style style);
u16  ui_input_u64        (uiid id, u64 *v, UI_Input_Style style);
void ui_quad             (Vector2 p0, Vector2 p1, Color32 color, f32 z = 0.0f);
void ui_line             (Vector2 start, Vector2 end, Color32 color, f32 z = 0.0f);
void ui_world_line       (Vector3 start, Vector3 end, Color32 color, f32 z = 0.0f);
u16  ui_combo            (uiid, u32 *selected_index, u32 option_count, const String *options, UI_Combo_Style style);
void ui_text             (String text, Vector2 pos, Color32 color, f32 z = 0.0f, const Font_Atlas *font_atlas = global_font_atlases.main_small);
void ui_text_with_shadow (String text, Vector2 pos, Color32 color, Vector2 shadow_offset, Color32 shadow_color, f32 z = 0.0f, const Font_Atlas *font_atlas = global_font_atlases.main_small);
