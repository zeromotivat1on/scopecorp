#pragma once

inline u64 Frame_index = 0;
inline f32 Delta_time  = 0.0f;
inline f32 Average_dt  = 0.0f;
inline f32 Average_fps = 0.0f;
inline u32 Draw_call_count = 0;

void r_update_stats();
