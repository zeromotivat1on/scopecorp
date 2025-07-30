#pragma once

typedef void *File;
typedef void(*For_Each_File_Callback)(const struct File_Callback_Data *callback_data);

extern const File INVALID_FILE;
extern const u32  FILE_FLAG_READ;
extern const u32  FILE_FLAG_WRITE;
extern const u32  FILE_OPEN_NEW;
extern const u32  FILE_OPEN_EXISTING;

struct File_Callback_Data {
    String path;
    void *user_data = null;
    u64 size = 0;
    u64 last_write_time = 0;
};

File os_open_file(String path, s32 open_type, u32 access_flags, bool log_error = true);
bool os_close_file(File handle);
s64  os_file_size(File handle);
bool os_read_file(File handle, void *buffer, u64 size, u64 *bytes_read = null);
bool os_write_file(File handle, const void *buffer, u64 size, u64 *bytes_written = null);
bool os_set_file_ptr(File handle, s64 position);
s64  os_file_ptr(File handle);

Buffer os_read_file(Arena &a, String path);
String os_read_text_file(Arena &a, String path);

void for_each_file(const char *directory, For_Each_File_Callback callback, void *user_data = null);

void extract_file_from_path(String &path);
void fix_directory_delimiters(String &path);
void remove_extension(String &path);
