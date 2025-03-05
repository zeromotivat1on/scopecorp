#include "pch.h"
#include "memory_storage.h"
#include "log.h"
#include "assertion.h"

#include "os/memory.h"

void *allocate_core() {
#if DEBUG
	void *vm_address = (void *)TB(2);
#else
	void *vm_address = null;
#endif

    constexpr u64 vm_size = GB(1);
	void* vm = vm_reserve(vm_address, vm_size);
    if (!vm) {
        error("Failed to reserve virtual address space");
        return null;
    }
    
    u8 *commited = (u8 *)vm_commit(vm, pers_memory_size + frame_memory_size + temp_memory_size);
    if (!commited) {
        error("Failed to commit application memory");
        return null;
    }
    
    static Memory_Storage p, f, t;
    pers  = &p;
    frame = &f;
    temp  = &t;

    u64 offset = 0;
    init(pers,  pers_memory_size,  commited + offset); offset += pers_memory_size;
    init(frame, frame_memory_size, commited + offset); offset += frame_memory_size;
    init(temp,  temp_memory_size,  commited + offset); offset += temp_memory_size;
    
    return vm;
}

void release_core(void *memory) {
	vm_release(memory);
}

void init(Memory_Storage *storage, u64 size, void* memory) {
	storage->data = (u8 *)vm_commit((u8 *)memory, size);
	storage->size = size;
	storage->used = 0;
}

void *push(Memory_Storage *storage, u64 size) {
	assert(storage->used + size <= storage->size);
	u8 *data = storage->data + storage->used;
	storage->used += size;
	return data;
}

void pop(Memory_Storage *storage, u64 size) {
	assert(storage->used >= size);
	storage->used -= size;
}

void clear(Memory_Storage *storage) {
	storage->used = 0;
}
