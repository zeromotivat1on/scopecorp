#pragma once

typedef void *File;
typedef void(*For_Each_File_Callback)(const struct File_Callback_Data *callback_data);

extern const File INVALID_FILE;
extern const u32  FILE_FLAG_READ;
extern const u32  FILE_FLAG_WRITE;
extern const u32  FILE_OPEN_NEW;
extern const u32  FILE_OPEN_EXISTING;

struct File_Callback_Data {
    const char *path = null;
    void *user_data  = null;
    u64 size = 0;
    u64 last_write_time = 0;
};

File open_file(const char *path, s32 open_type, u32 access_flags, bool log_error = true);
bool close_file(File handle);
s64  file_size(File handle);
bool read_file(File handle, void *buffer, u64 size, u64 *bytes_read = null);
bool write_file(File handle, void *buffer, u64 size, u64 *bytes_written = null);
bool set_file_pointer_position(File handle, s64 position);
s64  get_file_pointer_position(File handle);

void for_each_file(const char *directory, For_Each_File_Callback callback, void *user_data = null);

void extract_file_from_path(char *path);
void fix_directory_delimiters(char *path);
void remove_extension(char *path);

bool read_file(const char *path, void *buffer, u64 size, u64 *bytes_read = null, bool log_error = true);
