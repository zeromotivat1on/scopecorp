#pragma once

bool  read_file(const char* path, void* buffer, u64 buffer_size, u64* bytes_read);
void* read_entire_file_temp(const char* path, u64* bytes_read);

// Extract wave header data and return pointer to start of actual sound data.
void* extract_wav(void* data, s32* channel_count, s32* sample_rate, s32* bits_per_sample, s32* size);
