#pragma once

#include "hash_table.h"
#include "catalog.h"

inline const auto FLIP_BOOK_EXT = S("flip_book");

struct Texture;

struct Flip_Book_Frame {
    Texture *texture = null;
    f32 frame_time = 0.0f;
};

struct Flip_Book {
    String name;
    String path;
    
    u16 current_frame = 0;
    f32 frame_time = 0.0f;

    Array <Flip_Book_Frame> frames;
};

inline Table <String, Flip_Book> flip_book_table;

Flip_Book       *new_flip_book     (String path);
Flip_Book       *new_flip_book     (String path, String contents);
Flip_Book       *get_flip_book     (String name);
Flip_Book_Frame *get_current_frame (Flip_Book *flip_book);
void             update            (Flip_Book *flip_book, f32 dt);
