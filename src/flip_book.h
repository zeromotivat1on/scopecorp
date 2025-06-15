#pragma once

#include "asset.h"

inline constexpr u32 MAX_FLIP_BOOK_SIZE   = KB(2);
inline constexpr u16 MAX_FLIP_BOOK_FRAMES = 8;

typedef u64 sid;

struct Flip_Book : Asset {
    Flip_Book() { asset_type = ASSET_FLIP_BOOK; }
        
	sid frames[MAX_FLIP_BOOK_FRAMES];
	s32 frame_count = 0;
    s32 current_frame_index = 0;
	f32 switch_frame_time = F32_MAX; // seconds to switch to next frame
	f32 frame_time = 0.0f;
};

void init_flip_book_asset(Flip_Book *flip_book, void *data);

sid get_current_frame(Flip_Book *book);
sid advance_frame(Flip_Book *book);
void tick(Flip_Book *book, f32 dt);
