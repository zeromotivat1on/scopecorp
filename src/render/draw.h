#pragma once

struct Shader;
struct Texture;
struct Vertex_Buffer;
struct Index_Buffer;

enum Draw_Command_Flags : u32
{
    DRAW_FLAG_IGNORE_DEPTH = 0x00000001,
};

// @Cleanup: use handles to render primitives instead of pointers.
struct Draw_Command
{
    u32 flags = 0;
    s32 vertex_buffer_idx = INVALID_INDEX;
    s32 index_buffer_idx = INVALID_INDEX;
    s32 shader_idx = INVALID_INDEX;
    s32 texture_idx = INVALID_INDEX;
};

void draw(Draw_Command* cmd);
