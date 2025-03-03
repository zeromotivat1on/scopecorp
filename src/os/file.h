#pragma once

typedef void *File;

extern const File INVALID_FILE;
extern const s32  FILE_FLAG_READ;
extern const s32  FILE_FLAG_WRITE;
extern const s32  FILE_OPEN_NEW;
extern const s32  FILE_OPEN_EXISTING;

File open_file(const char *path, s32 open_type, s32 access_flags);
bool close_file(File handle);
s64  file_size(File handle);
bool read_file(File handle, void *buffer, u64 size, u64 *bytes_read);
bool write_file(File handle, void *buffer, u64 size, u64 *bytes_read);

bool  read_file(const char *path, void *buffer, u64 size, u64 *bytes_read);
void *read_entire_file_temp(const char *path, u64 *bytes_read);
