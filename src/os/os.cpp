#include "pch.h"
#include "log.h"
#include "str.h"
#include "os/file.h"

bool os_file_read(const char *path, void *buffer, u64 size, u64 *bytes_read, bool log_error) {
	File file = os_file_open(path, FILE_OPEN_EXISTING, FILE_FLAG_READ, log_error);
	if (file == INVALID_FILE) {
		return false;
	}

	defer { os_file_close(file); };

	if (bytes_read) *bytes_read = 0;
	if (!os_file_read(file, buffer, size, bytes_read)) {
		return false;
	}

	return true;
}

s64 os_file_get_size(const char *path) {
    File file = os_file_open(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
	if (file == INVALID_FILE) {
		return INVALID_INDEX;
	}

    defer { os_file_close(file); };

    return os_file_get_size(file);
}

void fix_directory_delimiters(char *path) {
    char *read  = path;
    char *write = path;
    bool last_was_slash = false;
    
    while (*read != '\0') {
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

    *write = '\0';
}

void remove_extension(char *path) {
    const char *dot   = str_char_from_end(path, '.');
    const s64 dot_pos = dot - path;
    path[dot_pos] = '\0';
}
