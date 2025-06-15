#pragma once

void *os_vm_reserve(void *addr, u64 size);
void *os_vm_commit(void *vm, u64 size);
bool  os_vm_decommit(void *vm, u64 size);
bool  os_vm_release(void *vm);
