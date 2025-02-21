#pragma once

bool  read_file(const char* path, void* buffer, u64 buffer_size, u64* bytes_read);
void* read_entire_file_temp(const char* path, u64* bytes_read);
