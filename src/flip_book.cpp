#include "pch.h"
#include "flip_book.h"
#include "assertion.h"

#include "render/texture.h"

#include <string.h>
#include <malloc.h>

void create_game_flip_books(Flip_Book_List *list) {
	// @Cleanup: looks like we should automate this.

	constexpr f32 frame_time = 0.15f;

	const s32 move_back_id_count = COUNT(texture_index_list.player_move[DIRECTION_BACK]);
	s32 *move_back_indices = (s32 *)alloca(move_back_id_count * sizeof(s32));
	for (s32 i = 0; i < move_back_id_count; ++i)
		move_back_indices[i] = texture_index_list.player_move[DIRECTION_BACK][i];
	list->player_move[DIRECTION_BACK] = create_flip_book(move_back_indices, move_back_id_count, frame_time);

	const s32 move_right_id_count = COUNT(texture_index_list.player_move[DIRECTION_RIGHT]);
	s32 *move_right_indices = (s32 *)alloca(move_right_id_count * sizeof(s32));
	for (s32 i = 0; i < move_right_id_count; ++i)
		move_right_indices[i] = texture_index_list.player_move[DIRECTION_RIGHT][i];
	list->player_move[DIRECTION_RIGHT] = create_flip_book(move_right_indices, move_right_id_count, frame_time);

	const s32 move_left_id_count = COUNT(texture_index_list.player_move[DIRECTION_LEFT]);
	s32 *move_left_indices = (s32 *)alloca(move_left_id_count * sizeof(s32));
	for (s32 i = 0; i < move_left_id_count; ++i)
		move_left_indices[i] = texture_index_list.player_move[DIRECTION_LEFT][i];
	list->player_move[DIRECTION_LEFT] = create_flip_book(move_left_indices, move_left_id_count, frame_time);

	const s32 move_forward_id_count = COUNT(texture_index_list.player_move[DIRECTION_FORWARD]);
	s32 *move_forward_indices = (s32 *)alloca(move_forward_id_count * sizeof(s32));
	for (s32 i = 0; i < move_forward_id_count; ++i)
		move_forward_indices[i] = texture_index_list.player_move[DIRECTION_FORWARD][i];
	list->player_move[DIRECTION_FORWARD] = create_flip_book(move_forward_indices, move_forward_id_count, frame_time);
}

Flip_Book create_flip_book(s32 *texture_indices, s32 count, f32 frame_time) {
	Flip_Book book = { 0 };
	book.frame_count = count;
	book.switch_frame_time = frame_time;
	memcpy(book.frames, texture_indices, count * sizeof(s32));
	return book;
}

s32 current_frame(Flip_Book *book) {
	assert(book->current_frame_index < book->frame_count);
	return book->frames[book->current_frame_index];
}

s32 advance_frame(Flip_Book *book) {
	assert(book->current_frame_index < book->frame_count);
	const u32 frame = book->frames[book->current_frame_index];
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
