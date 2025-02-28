#include "pch.h"
#include "os/memory.h"
#include "memory_storage.h"

constexpr u64 root_size = GB(1);
static void*  root_memory = null;

static Memory_Storage persistent_storage;
static Memory_Storage frame_storage;
static Memory_Storage temp_storage;

static u64 prealloc_offset = 0;

void prealloc_root() {
#if DEBUG
    void* root_address = (void*)TB(2);
#else
    void* root_address = null;
#endif

    root_memory = vm_reserve(root_address, root_size);
}

void free_root() {
    vm_release(root_memory);
}

void prealloc(Memory_Storage* storage, u64 size) {
    assert(prealloc_offset + size < root_size);
    
    storage->data = (u8*)vm_commit((u8*)root_memory + prealloc_offset, size);
    storage->size = size;
    storage->used = 0;

    prealloc_offset += size; 
}

void* alloc(Memory_Storage* storage, u64 size) {
    assert(storage->used + size <= storage->size);
    u8* data = storage->data + storage->used;
    storage->used += size;
    return data;
}

void free(Memory_Storage* storage, u64 size) {
    assert(storage->used >= size);
    storage->used -= size;
}

void free_all(Memory_Storage* storage) {
    storage->used = 0;
}

void prealloc_persistent(u64 size) {
    prealloc(&persistent_storage, size);
}

void* alloc_persistent(u64 size) {
    return alloc(&persistent_storage, size);
}

void free_persistent(u64 size) {
    free(&persistent_storage, size);
}

void free_all_persistent() {
    free_all(&persistent_storage);
}

void usage_persistent(u64* size, u64* used) {
    if (size) *size = persistent_storage.size;
    if (used) *used = persistent_storage.used;
}

void prealloc_frame(u64 size) {
    prealloc(&frame_storage, size);
}

void* alloc_frame(u64 size) {
    return alloc(&frame_storage, size);
}

void free_frame(u64 size) {
    free(&frame_storage, size);
}

void free_all_frame() {
    free_all(&frame_storage);
}

void usage_frame(u64* size, u64* used) {
    if (size) *size = frame_storage.size;
    if (used) *used = frame_storage.used;
}

void prealloc_temp(u64 size) {
    prealloc(&temp_storage, size);
}

void* alloc_temp(u64 size) {
    return alloc(&temp_storage, size);
}

void free_temp(u64 size) {
    free(&temp_storage, size);
}

void free_all_temp() {
    free_all(&temp_storage);
}

void usage_temp(u64* size, u64* used) {
    if (size) *size = temp_storage.size;
    if (used) *used = temp_storage.used;
}
