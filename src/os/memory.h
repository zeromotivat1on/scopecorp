#pragma once

void *os_reserve(void *addr, u64 size);
void *os_commit(void *vm, u64 size);
bool  os_decommit(void *vm, u64 size);
bool  os_release(void *vm);
