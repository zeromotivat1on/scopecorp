#pragma once

u64 get_page_size              ();
u64 get_allocation_granularity ();

void *heap_alloc (u64 size);
bool  heap_free  (void *p);

void *virtual_reserve  (void *addr, u64 size);
void *virtual_commit   (void *vm, u64 size);
bool  virtual_decommit (void *vm, u64 size);
bool  virtual_release  (void *vm);
