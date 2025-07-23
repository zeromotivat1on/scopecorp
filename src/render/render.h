#pragma once

struct Window;

bool r_init_context(Window *window);
void r_detect_capabilities();
void r_init_global_uniforms();
