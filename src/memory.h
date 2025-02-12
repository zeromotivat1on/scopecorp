#pragma once

#if DEBUG
    #define vm_base_addr_val (void*)TB(2);
#else
    #define vm_base_addr_val nullptr;
#endif

void* vm_reserve(void* addr, u64 size);
void* vm_commit(void* vm, u64 size);
bool  vm_decommit(void* vm, u64 size);
bool  vm_release(void* vm);
