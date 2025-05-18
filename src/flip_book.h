#pragma once

inline constexpr s32 MAX_FLIP_BOOK_FRAMES = 8;

typedef u64 sid;

struct Flip_Book {
	sid frames[MAX_FLIP_BOOK_FRAMES];
	s32 frame_count = 0;
    s32 current_frame_index = 0;
	f32 switch_frame_time = F32_MAX; // seconds to switch to next frame
	f32 frame_time = 0.0f;
};

struct Flip_Book_List {
	Flip_Book player_move[DIRECTION_COUNT];
};

inline Flip_Book_List flip_books;

void create_game_flip_books(Flip_Book_List *list);
Flip_Book create_flip_book(sid *texture_sids, s32 count, f32 frame_time);

sid get_current_frame(Flip_Book *book);
sid advance_frame(Flip_Book *book);
void tick(Flip_Book *book, f32 dt);
