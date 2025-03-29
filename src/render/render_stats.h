#pragma once

inline u64 frame_index = 0;
inline f32 delta_time  = 0.0f;
inline f32 average_dt  = 0.0f;
inline f32 average_fps = 0.0f;
inline s32 draw_call_count = 0;

void update_render_stats();
