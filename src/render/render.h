#pragma once

struct Window;
struct Frame_Buffer;

bool r_init_context(Window *window);
void r_detect_capabilities();
void r_init_global_uniforms();

void r_fb_submit_begin(const Frame_Buffer &frame_buffer);
void r_fb_submit_end(const Frame_Buffer &frame_buffer);
