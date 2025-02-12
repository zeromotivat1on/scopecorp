#include "pch.h"
#include "file.h"
#include "arena.h"
#include <stdio.h>

bool read_entire_file(const char* path, u8* buffer, u64 buffer_size, u64* bytes_read)
{
    FILE* handle;
    s32 error = fopen_s(&handle, path, "rb");
    if (error == 0)
    {
        const u64 file_size = fread(buffer, 1, buffer_size, handle);
        if (bytes_read) *bytes_read = file_size;
        error = ferror(handle);
        fclose(handle);
        if (error == 0) return true;
    }

    log("Failed to read file (%s)", path);
    if (bytes_read) *bytes_read = 0;
    return false;
}
