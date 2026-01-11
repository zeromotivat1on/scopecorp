#pragma once

#include "hash.h"
#include "hash_table.h"

inline constexpr u32 PAK_MAGIC     = U32_PACK('p', 'a', 'k', '0');
inline constexpr u32 PAK_TOC_MAGIC = U32_PACK('t', 'o', 'c', '0');
inline constexpr u32 PAK_VERSION   = 0;

// Pak format specification.
// 1. Pak_Header
// 2. Pure entry buffer data, no extra metadata.
// 3. Pak_Toc (table of contents)
// 4. Pak entries, but not exactly like Pak_Entry, layout for each entry is as follows:
//    - u16 : read entry size (bytes to read after, includes all data that goes below)
//    - u16 : entry name length
//    - len : entry name data (not null terminated)
//    - u32 : entry buffer data size
//    - u64 : entry buffer data offset from start of file (item 2 of specification)

enum Pak_Bits : u32 {
    
};

struct Pak_Header {
    u32 magic    = 0;
    u32 version  = 0;
    u32 bits     = 0;
    u32 reserved = 0;
    u64 table_of_contents_offset = 0;
};

struct Pak_Entry {
    static constexpr u16 MAX_NAME_LENGTH = 256;
    
    String name;
    Buffer buffer;
    u64 user_value;
    u64 offset_from_file_start = 0;
    
    static constexpr u16 MAX_READ_SIZE = sizeof(name.size) + MAX_NAME_LENGTH + sizeof(buffer.size) + sizeof(offset_from_file_start);
};

struct Pak_Toc {
    u32 magic       = 0;
    u32 entry_count = 0;
};

struct Create_Pak {
    Array<Pak_Entry> entries = { .allocator = __temporary_allocator };
};

struct Load_Pak {
    Allocator entry_data_allocator = __temporary_allocator;
    
    Pak_Header header;
    Pak_Toc    toc;
    
    Array <Pak_Entry>           entries = { .allocator = __temporary_allocator };
    Table <String, Pak_Entry *> lookup  = { .allocator = __temporary_allocator };
};

void add   (Create_Pak &pak, String entry_name, Buffer entry_buffer, u64 user_value = 0);
bool write (Create_Pak &pak, String path);

bool       load       (Load_Pak &pak, String path);
Pak_Entry *find_entry (Load_Pak &pak, String name);
