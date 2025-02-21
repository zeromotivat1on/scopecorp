#include "pch.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>

constexpr u64 root_size = GB(1);
static void*  root_memory = null;

static Memory_Storage persistent_storage;
static Memory_Storage frame_storage;
static Memory_Storage temp_storage;

static u64 prealloc_offset = 0;

void prealloc_root()
{
#if DEBUG
    void* root_address = (void*)TB(2);
#else
    void* root_address = null;
#endif

    root_memory = vm_reserve(root_address, root_size);
}

void free_root()
{
    vm_release(root_memory);
}

void prealloc_persistent(u64 size)
{
    persistent_storage.data = (u8*)vm_commit((u8*)root_memory + prealloc_offset, size);
    persistent_storage.size = size;
    persistent_storage.used = 0;

    prealloc_offset += size;
}

void* alloc_persistent(u64 size)
{
    assert(persistent_storage.used + size <= persistent_storage.size);
    u8* data = persistent_storage.data + persistent_storage.used;
    persistent_storage.used += size;
    return data;
}

void free_persistent(u64 size)
{
    assert(persistent_storage.used >= size);
    persistent_storage.used -= size;
}

void free_all_persistent()
{
    persistent_storage.used = 0;
}

void usage_persistent(u64* size, u64* used)
{
    if (size) *size = persistent_storage.size;
    if (used) *used = persistent_storage.used;
}

void prealloc_frame(u64 size)
{
    frame_storage.data = (u8*)vm_commit((u8*)root_memory + prealloc_offset, size);
    frame_storage.size = size;
    frame_storage.used = 0;

    prealloc_offset += size;
}

void* alloc_frame(u64 size)
{
    assert(frame_storage.used + size <= frame_storage.size);
    u8* data = frame_storage.data + frame_storage.used;
    frame_storage.used += size;
    return data;
}

void free_frame(u64 size)
{
    assert(frame_storage.used >= size);
    frame_storage.used -= size;
}

void free_all_frame()
{
    frame_storage.used = 0;    
}

void usage_frame(u64* size, u64* used)
{
    if (size) *size = frame_storage.size;
    if (used) *used = frame_storage.used;
}

void prealloc_temp(u64 size)
{
    temp_storage.data = (u8*)vm_commit((u8*)root_memory + prealloc_offset, size);
    temp_storage.size = size;
    temp_storage.used = 0;

    prealloc_offset += size;
}

void* alloc_temp(u64 size)
{
    assert(temp_storage.used + size <= temp_storage.size);
    u8* data = temp_storage.data + temp_storage.used;
    temp_storage.used += size;
    return data;
}

void free_temp(u64 size)
{
    assert(temp_storage.used >= size);
    temp_storage.used -= size;
}

void free_all_temp()
{
    temp_storage.used = 0;    
}

void usage_temp(u64* size, u64* used)
{
    if (size) *size = temp_storage.size;
    if (used) *used = temp_storage.used;
}

void log(const char* format, ...)
{   
    char buffer[4096] = {0};
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    puts(buffer);
}
