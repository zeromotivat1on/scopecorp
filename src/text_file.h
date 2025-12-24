#pragma once

#include "file_system.h"

// Convenient handler to work with text files, useful for custom formats for example.
struct Text_File_Handler {
    Allocator allocator = context.allocator;
    
    String path;
    File   file = FILE_NONE;

    String contents;
    s64    pos = 0;
};

inline void reset(Text_File_Handler *text) {
    if (text->file) {
        close_file(text->file);
        release(text->contents.data, text->allocator);
    }
    
    text->path     = {};
    text->contents = {};
    text->pos      = 0;
}

inline bool read_entire_file(Text_File_Handler *text, String path) {
    text->file = open_file(path, FILE_READ_BIT);
    if (!text->file) return false;

    text->contents = read_text_file(path, text->allocator);
    if (!text->contents) return false;
    
    text->path = path;
    text->pos  = 0;
    
    return true;
}

inline void init_from_memory(Text_File_Handler *text, String contents) {
    text->contents = contents;
    text->pos      = 0;
}

inline String read_line(Text_File_Handler *text) {
    auto s = slice(text->contents, text->pos);
    if (!s) return {};
    
    auto line = slice(s, S("\n"), S_LEFT_SLICE_BIT);
    if (!line) line = s; // get last part of string if did not find slice with new line
    
    text->pos += line.count;
    
    return line;
}
