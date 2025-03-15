#pragma once

enum Viewport_Aspect_Type {
    VIEWPORT_FILL_WINDOW,
    VIEWPORT_4X3,
};

struct Viewport {
    Viewport_Aspect_Type aspect_type = VIEWPORT_4X3;
    
    s16 x = 0;
    s16 y = 0;
    s16 width  = 0;
    s16 height = 0;
};

inline Viewport viewport;

struct vec3;

void resize_viewport(Viewport *viewport, s16 width, s16 height);
