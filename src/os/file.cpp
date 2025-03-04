#include "pch.h"
#include "file.h"
#include "log.h"
#include "memory_storage.h"

bool read_file(const char *path, void *buffer, u64 size, u64 *bytes_read)
{
	File file = open_file(path, FILE_OPEN_EXISTING, FILE_FLAG_READ);
	if (file == INVALID_FILE) {
		error("Failed to open file %s", path);
		return false;
	}

	defer { close_file(file); };

	if (bytes_read) *bytes_read = 0;
	if (!read_file(file, buffer, size, bytes_read)) {
		error("Failed to read file %s", path);
		return false;
	}

	return true;
}
