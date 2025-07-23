#pragma once

struct R_Flip_Book {
    static constexpr u32 MAX_FILE_SIZE = KB(64);
    static constexpr u32 MAX_FRAMES    = 8;
    
    struct Frame {
        u16 texture = 0;
    };
    
	u32 frame_count = 0;
    u32 current_frame_index = 0;

    f32 next_frame_time = 0.0f; // seconds to move to next frame
	f32 frame_time = 0.0f;

    Frame frames[MAX_FRAMES] = { 0 };

    struct Meta {
        u32 count = 0;
        f32 next_frame_time = 0.0f;
        sid textures[MAX_FRAMES] = { 0 }; // @Cleanup: extra memory is written to pak.
    };
};

u16  r_create_flip_book(u32 count, const u16 *textures, f32 next_frame_time);
void r_tick_flip_book(u16 flip_book, f32 dt);
R_Flip_Book::Frame &r_get_current_flip_book_frame(u16 flip_book);
R_Flip_Book::Frame &r_advance_flip_book_frame(u16 flip_book);
