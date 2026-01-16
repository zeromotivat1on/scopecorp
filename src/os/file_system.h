#pragma once

typedef void *File;

extern const File FILE_NONE;

enum File_Bits : u32 {
    // Access options.
    FILE_READ_BIT     = 0x1,
    FILE_WRITE_BIT    = 0x2,
    // Open options.
    FILE_NEW_BIT      = 0x4, // create new file if it does not exist
    FILE_TRUNCATE_BIT = 0x8, // same as FILE_NEW_BIT, but also truncates an existing writable file
};

struct File_Callback_Data {
    String path;
    void *user_data = null;
    u64 size = 0;
    u64 last_write_time = 0;
};

String get_process_directory ();
void   set_process_cwd       (String path);

Array <Source_Code_Location> get_current_callstack (); // returned array is temp allocated

File   open_file        (String path, u32 bits, bool log_error = true);
bool   close_file       (File handle);
u64    get_file_size    (File handle);
u64    read_file        (File handle, u64 size, void *buffer);
u64    write_file       (File handle, u64 size, const void *buffer);
s64    get_file_ptr     (File handle);
bool   set_file_ptr     (File handle, s64 position);
bool   path_file_exists (String path);
Buffer read_file        (String path, Allocator alc);
String read_text_file   (String path, Allocator alc);
void   write_file       (String path, Buffer buffer);
void   write_text_file  (String path, String source);
void   visit_directory  (String path, void (*callback) (const File_Callback_Data *),
                         bool recursive = true, void *user_data = null);

String extract_file_from_path   (String path, Allocator alc);
String fix_directory_delimiters (String path);
String remove_extension         (String path);
