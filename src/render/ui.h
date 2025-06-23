#pragma once

#include "math/vector.h"

inline constexpr u8  MAX_UI_FONT_ATLASES = 32;
inline constexpr u16 MAX_UI_DRAW_QUEUE_SIZE = 1024;
inline constexpr u16 MAX_UI_TEXT_DRAW_BUFFERS = 8;
inline constexpr u16 MAX_UI_TEXT_DRAW_BUFFER_CHARS = 4096;
inline constexpr u16 MAX_UI_QUAD_DRAW_BUFFER_QUADS = 512;

inline constexpr s32 UI_DEFAULT_FONT_ATLAS_INDEX = 0;
inline constexpr s32 UI_DEBUG_CONSOLE_FONT_ATLAS_INDEX = 0;
inline constexpr s32 UI_PROFILER_FONT_ATLAS_INDEX = 0;
inline constexpr s32 UI_SCREEN_REPORT_FONT_ATLAS_INDEX = 1;

struct mat4;
struct Font_Atlas;

enum UI_Draw_Type {
    UI_DRAW_TEXT,
    UI_DRAW_QUAD,
};

struct UI_Draw_Command {
    UI_Draw_Type type;
    s32 element_count = 0;
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
    Font_Atlas *font_atlases[MAX_UI_FONT_ATLASES];
    UI_Draw_Command draw_queue[MAX_UI_DRAW_QUEUE_SIZE];

    u8  font_atlas_count = 0;
    u16 draw_queue_size  = 0;

    UI_Text_Draw_Buffer text_draw_buffer;
    UI_Quad_Draw_Buffer quad_draw_buffer;
};

inline UI ui;

void ui_init();
void ui_draw_text(const char *text, vec2 pos, u32 color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_draw_text(const char *text, u32 count, vec2 pos, u32 color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_draw_text_with_shadow(const char *text, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_draw_text_with_shadow(const char *text, u32 count, vec2 pos, u32 color, vec2 shadow_offset, u32 shadow_color, s32 atlas_index = UI_DEFAULT_FONT_ATLAS_INDEX);
void ui_draw_quad(vec2 p0, vec2 p1, u32 color);
void ui_flush();
