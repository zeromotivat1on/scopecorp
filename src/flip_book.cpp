#include "pch.h"
#include "flip_book.h"

#include "render/texture.h"

#include <string.h>

void create_game_flip_books(Flip_Book_List *list) {
	// @Cleanup: looks like we should automate this.

	constexpr f32 frame_time = 0.15f;

    for (s32 d = 0; d < DIRECTION_COUNT; ++d) {
        sid move_sids[MAX_PLAYER_MOVE_FRAMES];
        const s32 move_sid_count = COUNT(texture_sids.player_move[d]);
        
        for (s32 i = 0; i < move_sid_count; ++i)
            move_sids[i] = texture_sids.player_move[d][i];
        
        list->player_move[d] = create_flip_book(move_sids, move_sid_count, frame_time);
    }
}

Flip_Book create_flip_book(sid *texture_sids, s32 count, f32 frame_time) {
	Flip_Book book = {};
	book.frame_count = count;
	book.switch_frame_time = frame_time;
	memcpy(book.frames, texture_sids, count * sizeof(sid));
	return book;
}

sid current_frame(Flip_Book *book) {
	Assert(book->current_frame_index < book->frame_count);
	return book->frames[book->current_frame_index];
}

sid advance_frame(Flip_Book *book) {
	Assert(book->current_frame_index < book->frame_count);
	const sid frame = book->frames[book->current_frame_index];
	book->current_frame_index = (book->current_frame_index + 1) % book->frame_count;
	return frame;
}

void tick(Flip_Book *book, f32 dt) {
	book->frame_time += dt;
	if (book->frame_time > book->switch_frame_time) {
		advance_frame(book);
		book->frame_time = 0.0f;
	}
}
