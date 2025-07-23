#pragma once

struct R_Pass {    
    struct Rect {
        s32 x = 0;
        s32 y = 0;
        u32 w = 0;
        u32 h = 0;
    };
    
    struct Clear {
        u32 color = 0;
        u32 bits = R_NONE;
    };
    
    struct Cull {
        u32 face = R_NONE;
        u32 winding = R_NONE;
    };

    struct Blend {
        u32 src = R_NONE;
        u32 dst = R_NONE;
    };

    struct Depth {
        u32 mask = R_NONE;
        u32 func = R_NONE;
    };

    struct Stencil {
        struct Func {
            u32 type = R_NONE;
            u32 comparator = 0;
            u32 mask = 0;
        };

        struct Op {
            u32 stencil_failed = R_NONE;
            u32 depth_failed = R_NONE;
            u32 passed = R_NONE;
        };

        u32 mask = R_NONE;
        Func func;
        Op op;
    };

    u32 polygon = R_NONE;
    Rect viewport;
    Rect scissor;
    Clear clear;
    Cull cull;
    Blend blend;
    Depth depth;
    Stencil stencil;
};

void r_submit(const R_Pass &pass);
