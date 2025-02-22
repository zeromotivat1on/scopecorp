#pragma once

inline constexpr s32 MAX_FLIP_BOOK_FRAMES = 8;

struct Flip_Book
{
    s32 frames[MAX_FLIP_BOOK_FRAMES]; // texture indices from render registry
    s32 frame_count;
    s32 current_frame_idx;
    f32 switch_frame_time; // amount of time to switch to next frame
    f32 frame_time;
};

struct Flip_Book_List
{
    Flip_Book player_move[DIRECTION_COUNT];
};

inline Flip_Book_List flip_books;

void create_game_flip_books(Flip_Book_List* list);
Flip_Book create_flip_book(s32* texture_indices, s32 count, f32 frame_time);

s32 current_frame(Flip_Book* book);
s32 advance_frame(Flip_Book* book);
void tick(Flip_Book* book, f32 dt);
