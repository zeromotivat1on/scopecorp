#pragma once

enum Viewport_Aspect_Type {
    VIEWPORT_FILL_WINDOW,
    VIEWPORT_4X3,
};

struct Viewport {
    // @Todo: fix render for 4x3 aspect ratio.
    Viewport_Aspect_Type aspect_type = VIEWPORT_FILL_WINDOW;
    
    s16 x = 0;
    s16 y = 0;
    s16 width  = 0;
    s16 height = 0;
    s32 frame_buffer_index = INVALID_INDEX;
};

inline Viewport viewport;

void resize_viewport(Viewport *viewport, s16 width, s16 height);
