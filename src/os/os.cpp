#include "pch.h"
#include "file_system.h"

Buffer read_file(String path, Allocator alc) {
    auto file = open_file(path, FILE_READ_BIT);
	if (file == FILE_NONE) return {};
	defer { close_file(file); };

    const u64 size = get_file_size(file);
    if (size == INDEX_NONE) return {};

    void *data = alloc(size, alc);
    if (!read_file(file, size, data)) {
        release(data);
        return {};
    }

    return { .data = (u8 *)data, .size = size };
}

String read_text_file(String path, Allocator alc) {
    auto buffer = read_file(path, alc);
    if (is_valid(buffer)) return make_string(buffer);
    return {};
}

void write_file(String path, Buffer buffer) {
    auto file = open_file(path, FILE_WRITE_BIT | FILE_TRUNCATE_BIT, false);
    if (file == FILE_NONE) return;
    
	defer { close_file(file); };

    const auto written = write_file(file, buffer.size, buffer.data);
    if (written != buffer.size) {
        log(LOG_WARNING, "Partially wrote %llu/%llu bytes to %S", written, buffer.size, path);
    }
}

void write_text_file(String path, String source) {
    write_file(path, make_buffer(source));
}

String fix_directory_delimiters(String path) {
    auto read  = path.data;
    auto write = path.data;
    bool last_was_slash = false;
    
    while (read != path.data + path.size) {
        if (*read == '\\') *read = '/';
        if (*read == '/') {
            if (!last_was_slash) {
                *write++ = *read;
                last_was_slash = true;
            }
        } else {
            *write++ = *read;
            last_was_slash = false;
        }
        read++;
    }

    path.size = write - path.data;
    
    return path;
}

String remove_extension(String path) {
    const s64 index = find(path, '.', S_SEARCH_REVERSE_BIT);
    if (index != INDEX_NONE) {
        path.size = index;
        return path;
    }

    return {};
}
