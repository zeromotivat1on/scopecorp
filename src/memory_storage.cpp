#include "pch.h"
#include "memory_storage.h"
#include "log.h"

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
    
    u8 *commited = (u8 *)vm_commit(vm, PERS_MEMORY_SIZE + FRAME_MEMORY_SIZE + TEMP_MEMORY_SIZE);
    if (!commited) {
        error("Failed to commit application memory");
        return null;
    }
    
    static Memory_Storage _pers, _frame, _temp, _panic;
    pers  = &_pers;
    frame = &_frame;
    temp  = &_temp;
    panic = &_panic;
    
    u64 offset = 0;
    init(pers,  PERS_MEMORY_SIZE,  commited + offset); offset += PERS_MEMORY_SIZE;
    init(frame, FRAME_MEMORY_SIZE, commited + offset); offset += FRAME_MEMORY_SIZE;
    init(temp,  TEMP_MEMORY_SIZE,  commited + offset); offset += TEMP_MEMORY_SIZE;
    init(panic, PANIC_MEMORY_SIZE, commited + offset); offset += PANIC_MEMORY_SIZE;
    
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
	Assert(storage->used + size <= storage->size);
	u8 *data = storage->data + storage->used;
	storage->used += size;
	return data;
}

void pop(Memory_Storage *storage, u64 size) {
	Assert(storage->used >= size);
	storage->used -= size;
}

void clear(Memory_Storage *storage) {
	storage->used = 0;
}
