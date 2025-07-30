#include "pch.h"
#include "log.h"
#include "str.h"
#include "os/file.h"

bool os_read_file(String path, void *buffer, u64 size, u64 *bytes_read, bool log_error) {
	File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ, log_error);
	if (file == INVALID_FILE) {
		return false;
	}

	defer { os_close_file(file); };

	if (bytes_read) *bytes_read = 0;
	if (!os_read_file(file, buffer, size, bytes_read)) {
		return false;
	}

	return true;
}

Buffer os_read_file(Arena &a, String path) {
    File file = os_open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
	if (file == INVALID_FILE) {
		return BUFFER_NONE;
	}

	defer { os_close_file(file); };

    const u64 size = os_file_size(file);
    Buffer buffer = arena_push_buffer(a, size);

    if (!os_read_file(file, buffer.data, buffer.size)) {
        pop(a, size);
        return BUFFER_NONE;
    }

    return buffer;
}

String os_read_text_file(Arena &a, String path) {
    Buffer buffer = os_read_file(a, path);
    if (is_valid(buffer)) {
        return String { (char *)buffer.data, buffer.size };
    }

    return STRING_NONE;
}

void fix_directory_delimiters(String &path) {
    char *read  = path.value;
    char *write = path.value;
    bool last_was_slash = false;
    
    while (read != path.value + path.length) {
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

    path.length = write - path.value;
}

void remove_extension(String &path) {
    const s64 index = str_index(path, '.', S_SEARCH_REVERSE_BIT);
    if (index != INDEX_NONE) {
        path.length = index;
    }
}
