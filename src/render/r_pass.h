#pragma once

struct R_Pass {    
    struct Viewport {
        s32 x = 0;
        s32 y = 0;
        u32 w = 0;
        u32 h = 0;
    };

    struct Scissor {
        u16 test = R_NONE;
        s32 x = 0;
        s32 y = 0;
        u32 w = 0;
        u32 h = 0;
    };
    
    struct Cull {
        u16 test = R_NONE;
        u16 face = R_NONE;
        u16 winding = R_NONE;
    };

    struct Blend {
        u16 test = R_NONE;
        u16 src = R_NONE;
        u16 dst = R_NONE;
    };

    struct Depth {
        u16 test = R_NONE;
        u16 mask = R_NONE;
        u16 func = R_NONE;
    };

    struct Stencil {
        struct Func {
            u16 type = R_NONE;
            u32 comparator = 0;
            u32 mask = 0;
        };

        struct Op {
            u16 stencil_failed = R_NONE;
            u16 depth_failed = R_NONE;
            u16 passed = R_NONE;
        };

        u16 test = R_NONE;
        u32 mask = 0;
        Func func;
        Op op;
    };

    struct Clear {
        u32 color = 0;
        u32 bits = R_NONE;
    };
    
    u16 polygon = R_NONE;
    Viewport viewport;
    Scissor scissor;
    Cull cull;
    Blend blend;
    Depth depth;
    Stencil stencil;
    Clear clear;
};

void r_submit(const R_Pass &pass);
