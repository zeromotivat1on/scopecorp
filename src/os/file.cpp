#include "pch.h"
#include "file.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

bool read_file(const char* path, void* buffer, u64 buffer_size, u64* bytes_read)
{
    FILE* file;
    s32 err = fopen_s(&file, path, "rb");
    if (err == 0)
    {
        const u64 read_count = fread(buffer, 1, buffer_size, file);
        if (bytes_read) *bytes_read = read_count;
        err = ferror(file);
        fclose(file);
        if (err == 0) return true;
    }

    error("Failed to read file %s with error %d %s", path, err, strerror(err));
    if (bytes_read) *bytes_read = 0;
    
    return false;
}

void* read_entire_file_temp(const char* path, u64* bytes_read)
{
    FILE* file;
    s32 err = fopen_s(&file, path, "rb");
    if (err == 0)
    {
        fseek(file, 0, SEEK_END);
        const u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);

        void* buffer = alloc_temp(size);
        const u64 read_count = fread(buffer, 1, size, file);
        if (bytes_read) *bytes_read = read_count;

        err = ferror(file);
        fclose(file);
        if (err == 0) return buffer;

        free_temp(size);
    }

    error("Failed to read entire file temp %s with error %d %s", path, err, strerror(err));
    if (bytes_read) *bytes_read = 0;
    
    return null;
}
