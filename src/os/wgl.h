#pragma once

void wgl_init(struct Window* window, s32 major_version, s32 minor_version);
void wgl_swap_buffers(struct Window* window);
void wgl_vsync(bool enable);
