#pragma once

#include "font.h"

#include "math/vector.h"

#include "render/r_pass.h"
#include "render/r_command.h"

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

struct mat4;
struct Font_Atlas;

// Bits that are returned from immediate ui elements.
enum UI_Immediate_Bits : u16 {
    UI_HOT_BIT       = 0x1,
    UI_UNHOT_BIT     = 0x2,
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

bool operator==(const uiid &a, const uiid &b);
bool operator!=(const uiid &a, const uiid &b);

struct UI_Color {
    u32 cold   = 0;
    u32 hot    = 0;
    u32 active = 0;
};

struct UI_Button_Style {
    f32 z = 0.0f;
    vec2 pos_text = vec2_zero;
    vec2 padding  = vec2_zero;
    UI_Color color_text;
    UI_Color color_quad;
    s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX;
};

struct UI_Input_Style {
    f32 z = 0.0f;
    vec2 pos_text = vec2_zero;
    vec2 padding  = vec2_zero;
    UI_Color color_text;
    UI_Color color_quad;
    UI_Color color_cursor;
    s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX;
};

struct UI_Combo_Style {
    f32 z = 0.0f;
    vec2 pos_text = vec2_zero;
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

struct R_UI {
    static constexpr u32 MAX_COMMANDS = 512;
    static constexpr u32 MAX_FONT_ATLASES = 32;
    
    struct Line_Render {
        static constexpr u16 MAX_LINES = 256;
    
        vec2 *positions = null;
        u32  *colors    = null;

        u16 line_count = 0;

        u16 vertex_desc = 0;
        u16 material = 0;
    };

    struct Quad_Render {
        static constexpr u16 MAX_QUADS = 128;
    
        vec2 *positions = null;
        u32  *colors    = null;

        u16 quad_count = 0;

        u16 vertex_desc = 0;
        u16 material = 0;
    };

    struct Text_Render {
        static constexpr u16 MAX_CHARS = 2048;

        f32  *positions  = null;
        u32  *colors     = null;
        u32  *charmap    = null;
        mat4 *transforms = null;

        u16 char_count = 0;

        u16 vertex_desc = 0;
        u16 material = 0;
    };
    
    uiid id_hot;
    uiid id_active;
    
    u16 font_atlas_count = 0;
    Font_Atlas font_atlases[MAX_FONT_ATLASES];

    Line_Render line_render;
    Text_Render text_render;
    Quad_Render quad_render;

    R_Pass pass;
    R_Command_List command_list;
};

inline R_UI R_ui;

void ui_init();
void ui_flush();

u16 ui_button(uiid id, String text, const UI_Button_Style &style);
u16 ui_input_text(uiid id, char *text, u32 size, const UI_Input_Style &style);

u16 ui_input_f32(uiid id, f32 *v, const UI_Input_Style &style);

u16 ui_input_s8 (uiid id, s8  *v, const UI_Input_Style &style);
u16 ui_input_s16(uiid id, s16 *v, const UI_Input_Style &style);
u16 ui_input_s32(uiid id, s32 *v, const UI_Input_Style &style);
u16 ui_input_s64(uiid id, s64 *v, const UI_Input_Style &style);

u16 ui_input_u8 (uiid id, u8  *v, const UI_Input_Style &style);
u16 ui_input_u16(uiid id, u16 *v, const UI_Input_Style &style);
u16 ui_input_u32(uiid id, u32 *v, const UI_Input_Style &style);
u16 ui_input_u64(uiid id, u64 *v, const UI_Input_Style &style);

u16 ui_input_sid(uiid id, sid *v, const UI_Input_Style &style);

u16 ui_combo(uiid, u32 *selected_index, u32 option_count, const String *options, const UI_Combo_Style &style);

void ui_text(String text, vec2 pos, u32 color, f32 z = 0.0f, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_text_with_shadow(String text, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, f32 z = 0.0f, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);

void ui_quad(vec2 p0, vec2 p1, u32 color, f32 z = 0.0f);

void ui_line(vec2 start, vec2 end, u32 color, f32 z = 0.0f);
void ui_world_line(vec3 start, vec3 end, u32 color, f32 z = 0.0f);
