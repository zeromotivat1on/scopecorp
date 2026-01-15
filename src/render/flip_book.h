#pragma once

#include "hash_table.h"
#include "catalog.h"

inline const auto FLIP_BOOK_EXT = S("flip_book");

struct Texture;

struct Flip_Book_Frame {
    Atom texture;
    f32  frame_time;
};

struct Flip_Book {
    f32                     update_time;
    u32                     current_frame;
    Array <Flip_Book_Frame> frames;
};

inline Table <Atom, Flip_Book> flip_book_table;

Flip_Book       *new_flip_book     (String path);
Flip_Book       *new_flip_book     (Atom name, String contents);
Flip_Book       *get_flip_book     (Atom name);
Flip_Book_Frame *get_current_frame (Flip_Book *flip_book);
void             update            (Flip_Book *flip_book, f32 dt);
