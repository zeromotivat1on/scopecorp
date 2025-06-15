#include "pch.h"
#include "flip_book.h"
#include "str.h"
#include "log.h"

#include "render/texture.h"

void init_flip_book_asset(Flip_Book *flip_book, void *data) {
      enum Declaration_Type {
        DECL_NONE,
        DECL_TEXTURE,
        DECL_FRAME_TIME,
    };

    constexpr u32 MAX_LINE_BUFFER_SIZE = 256;

    const char *DECL_NAME_TEXTURE = "t";
    const char *DECL_NAME_FRAME_TIME = "ft";

    const char *DELIMITERS = " ";
    
    flip_book->frame_count = 0;
    
    char *p = (char *)data;
    p[flip_book->data_size] = '\0';

    char line_buffer[MAX_LINE_BUFFER_SIZE];
    char *new_line = str_char(p, ASCII_NEW_LINE);
    
    while (new_line) {
        const s64 line_size = new_line - p;
        Assert(line_size < MAX_LINE_BUFFER_SIZE);
        str_copy(line_buffer, p, line_size);
        line_buffer[line_size] = '\0';
        
        char *line = str_trim(line_buffer);
        if (str_size(line) > 0) {
            char *token = str_token(line, DELIMITERS);
            Declaration_Type decl_type = DECL_NONE;

            if (str_cmp(token, DECL_NAME_TEXTURE)) {
                decl_type = DECL_TEXTURE;
            } else if (str_cmp(token, DECL_NAME_FRAME_TIME)) {
                decl_type = DECL_FRAME_TIME;
            } else {
                error("Unknown flip book token declaration '%s'", token);
                continue;
            }

            switch (decl_type) {
            case DECL_TEXTURE: {
                const char *sv = str_token(null, DELIMITERS);

                Assert(flip_book->frame_count < MAX_FLIP_BOOK_FRAMES);
                flip_book->frames[flip_book->frame_count] = sid_cache(sv);
                flip_book->frame_count += 1;
                
                break;
            }
            case DECL_FRAME_TIME: {
                const char *sv = str_token(null, DELIMITERS);
                const f32 v = str_to_f32(sv);
                flip_book->switch_frame_time = v;
                
                break;
            }
            }
        }
        
        p += line_size + 1;
        new_line = str_char(p, ASCII_NEW_LINE);
    }
}

sid get_current_frame(Flip_Book *book) {
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
