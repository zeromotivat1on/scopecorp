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

String fix_directory_delimiters(String path) {
    char *read  = path.data;
    char *write = path.data;
    bool last_was_slash = false;
    
    while (read != path.data + path.count) {
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

    path.count = write - path.data;
    
    return path;
}

String remove_extension(String path) {
    const s64 index = find(path, '.', S_SEARCH_REVERSE_BIT);
    if (index != INDEX_NONE) {
        path.count = index;
        return path;
    }

    return {};
}
