#pragma once

struct Viewport {
    s16 x;
    s16 y;
    s16 width;
    s16 height;
};

inline Viewport viewport;

inline void viewport_4x3(Viewport* viewport, s16 window_w, s16 window_h) {
    assert(viewport);
    
    viewport->width = window_w;
    viewport->height = window_h;
    
    if (window_w * 3 > window_h * 4) {
        viewport->width = window_h * 4 / 3;
        viewport->x = (window_w - viewport->width) / 2;
    } else {
        viewport->height = window_w * 3 / 4;
        viewport->y = (window_h - viewport->height) / 2;
    }
}
