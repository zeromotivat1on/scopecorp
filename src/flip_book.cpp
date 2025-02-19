#include "pch.h"
#include "flip_book.h"
#include "render/texture.h"
#include <string.h>
#include <malloc.h>

void create_game_flip_books(Flip_Book_List* list)
{
    // @Cleanup: looks like we should automate this.

    constexpr f32 frame_time = 0.15f;
    
    const s32 move_back_id_count = c_array_count(textures.player_move[BACK]);
    u32* move_back_ids = (u32*)alloca(move_back_id_count * sizeof(u32));
    for (s32 i = 0; i < move_back_id_count; ++i)
        move_back_ids[i] = textures.player_move[BACK][i].id;
    list->player_move[BACK] = create_flip_book(move_back_ids, move_back_id_count, frame_time);

    const s32 move_right_id_count = c_array_count(textures.player_move[RIGHT]);
    u32* move_right_ids = (u32*)alloca(move_right_id_count * sizeof(u32));
    for (s32 i = 0; i < move_right_id_count; ++i)
        move_right_ids[i] = textures.player_move[RIGHT][i].id;
    list->player_move[RIGHT] = create_flip_book(move_right_ids, move_right_id_count, frame_time);

    const s32 move_left_id_count = c_array_count(textures.player_move[LEFT]);
    u32* move_left_ids = (u32*)alloca(move_left_id_count * sizeof(u32));
    for (s32 i = 0; i < move_left_id_count; ++i)
        move_left_ids[i] = textures.player_move[LEFT][i].id;
    list->player_move[LEFT] = create_flip_book(move_left_ids, move_left_id_count, frame_time);

    const s32 move_forward_id_count = c_array_count(textures.player_move[FORWARD]);
    u32* move_forward_ids = (u32*)alloca(move_forward_id_count * sizeof(u32));
    for (s32 i = 0; i < move_forward_id_count; ++i)
        move_forward_ids[i] = textures.player_move[FORWARD][i].id;
    list->player_move[FORWARD] = create_flip_book(move_forward_ids, move_forward_id_count, frame_time);
}

Flip_Book create_flip_book(u32* texture_ids, s32 count, f32 frame_time)
{    
    Flip_Book book = {0};
    book.frame_count = count;
    book.switch_frame_time = frame_time;
    memcpy(book.frames, texture_ids, count * sizeof(u32));
    return book;
}

u32 current_frame(Flip_Book* book)
{
    assert(book->current_frame_idx < book->frame_count);
    return book->frames[book->current_frame_idx];
}

u32 advance_frame(Flip_Book* book)
{
    assert(book->current_frame_idx < book->frame_count);
    const u32 frame = book->frames[book->current_frame_idx];
    book->current_frame_idx = (book->current_frame_idx + 1) % book->frame_count;
    return frame;
}

void tick(Flip_Book* book, f32 dt)
{
    book->frame_time += dt;
    if (book->frame_time > book->switch_frame_time)
    {
        advance_frame(book);
        book->frame_time = 0.0f;
    }
}
