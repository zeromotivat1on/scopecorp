#pragma once

#include "file_system.h"

// Convenient handler to work with text files, useful for custom formats for example.
struct Text_File_Handler {
    Allocator allocator = context.allocator;
    String    path;
    File      file = FILE_NONE;
    String    contents;
    s64       pos = 0;
};

inline void reset(Text_File_Handler *handler) {
    if (handler->file) {
        close_file(handler->file);
        release(handler->contents.data, handler->allocator);
    }
    
    handler->path     = {};
    handler->contents = {};
    handler->pos      = 0;
}

inline bool read_entire_file(Text_File_Handler *handler, String path) {
    handler->file = open_file(path, FILE_READ_BIT);
    if (!handler->file) return false;

    const auto size = get_file_size(handler->file);
    handler->contents.data = (u8 *)alloc(size, handler->allocator);
    handler->contents.size = read_file(handler->file, size, handler->contents.data);

    if (size != handler->contents.size) {
        log(LOG_WARNING, "Read different amount of bytes %llu/%llu from %S", handler->contents.size, size, path);
    }
    
    handler->path = path;
    handler->pos  = 0;
    
    return true;
}

inline void init_from_memory(Text_File_Handler *handler, String contents) {
    handler->contents = contents;
    handler->pos      = 0;
}

inline String read_line(Text_File_Handler *handler) {
    auto s = slice(handler->contents, handler->pos);
    if (!s) return {};
    
    auto line = slice(s, S("\n"), S_LEFT_SLICE_BIT);
    if (!line) line = s; // get last part of string if did not find slice with new line
    
    handler->pos += line.size;
    
    return line;
}
