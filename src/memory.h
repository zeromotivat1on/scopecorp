#pragma once

struct Memory_Storage
{
    u8* data;
    u64 size;
    u64 used;
};

void* vm_reserve(void* addr, u64 size);
void* vm_commit(void* vm, u64 size);
bool  vm_decommit(void* vm, u64 size);
bool  vm_release(void* vm);
