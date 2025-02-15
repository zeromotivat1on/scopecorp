#include "pch.h"
#include "file.h"
#include "memory_eater.h"
#include <stdio.h>
#include <string.h>

bool read_file(const char* path, void* buffer, u64 buffer_size, u64* bytes_read)
{
    FILE* file;
    s32 error = fopen_s(&file, path, "rb");
    if (error == 0)
    {
        const u64 read_count = fread(buffer, 1, buffer_size, file);
        if (bytes_read) *bytes_read = read_count;
        error = ferror(file);
        fclose(file);
        if (error == 0) return true;
    }

    log("Failed to read file (%s)", path);
    if (bytes_read) *bytes_read = 0;
    
    return false;
}

void* read_entire_file_temp(const char* path, u64* bytes_read)
{
    FILE* file;
    s32 error = fopen_s(&file, path, "rb");
    if (error == 0)
    {
        fseek(file, 0, SEEK_END);
        const u64 size = ftell(file);
        fseek(file, 0, SEEK_SET);

        void* buffer = alloc_temp(size);
        const u64 read_count = fread(buffer, 1, size, file);
        if (bytes_read) *bytes_read = read_count;

        error = ferror(file);
        fclose(file);
        if (error == 0) return buffer;

        free_temp(size);
    }

    log("Failed to read file (%s)", path);
    if (bytes_read) *bytes_read = 0;
    
    return null;
}

void* extract_wav(void* data, s32* channel_count, s32* sample_rate, s32* bits_per_sample, s32* size)
{   
    const char* riff = (char*)eat(&data, 4);
    if (strncmp(riff, "RIFF", 4) != 0)
    {
        log("File is not a valid wave file, header does not begin with RIFF");
        return null;
    }

    eat_s32(&data); // file size

    const char* wave_str = (char*)eat(&data, 4);
    if (strncmp(wave_str, "WAVE", 4) != 0)
    {
        log("File is not a valid wave file, header does not contain WAVE");
        return null;
    }

    const char* fmt_str = (char*)eat(&data, 4);
    if (strncmp(fmt_str, "fmt", 3) != 0)
    {
        log("File is not a valid wave file, header does not contain FMT");
        return null;
    }
    
    const s32 fmt_size = eat_s32(&data);
    if (fmt_size != 16)
    {
        log("File is not a valid wave file, fmt size is not 16");
        return null;
    }
    
    eat_s16(&data); // audio format
    
    if (channel_count) *channel_count = eat_s16(&data);
    if (sample_rate) *sample_rate = eat_s32(&data);

    eat_s32(&data); // byte rate
    eat_s16(&data); // block align

    if (bits_per_sample) *bits_per_sample = eat_s16(&data);

    const char* data_str = (char*)eat(&data, 4);
    if (strncmp(data_str, "data", 4) != 0)
    {
        log("File is not a valid wave file, header does not contain DATA");
        return null;
    }

    if (size) *size = eat_s32(&data);
    
    return data;
}
