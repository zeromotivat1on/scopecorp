#pragma once

// Replace with own opengl declarations?
#include "glad.h"

void gl_init(struct Window* window, s32 major_version, s32 minor_version);
void gl_swap_buffers(struct Window* window);
void gl_vsync(bool enable);

u32 gl_create_shader(GLenum type, const char* src);
u32 gl_link_program(u32 vertex_shader, u32 fragment_shader);
u32 gl_create_program(const char* vertex_path, const char* fragment_path);

u32 gl_create_texture(void* data, s32 width, s32 height);
