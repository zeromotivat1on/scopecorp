#pragma once

typedef void *File;
typedef void(*For_Each_File_Callback)(const struct File_Callback_Data *callback_data);

extern const File FILE_NONE;
extern const u32  FILE_READ_BIT;
extern const u32  FILE_WRITE_BIT;
extern const u32  FILE_OPEN_NEW;
extern const u32  FILE_OPEN_EXISTING;

struct File_Callback_Data {
    String path;
    void *user_data = null;
    u64 size = 0;
    u64 last_write_time = 0;
};

File os_open_file(String path, s32 open_type, u32 access_bits);
bool os_close_file(File handle);
u64  os_file_size(File handle);
u64  os_read_file(File handle, u64 size, void *buffer);
u64  os_write_file(File handle, u64 size, const void *buffer);
s64  os_file_ptr(File handle);
bool os_set_file_ptr(File handle, s64 position);

Buffer os_read_file(Arena &a, String path);
String os_read_text_file(Arena &a, String path);

void for_each_file(const char *directory, For_Each_File_Callback callback, void *user_data = null);

void extract_file_from_path(String &path);
void fix_directory_delimiters(String &path);
void remove_extension(String &path);
