#pragma once

struct Memory_Storage {
    u8* data;
    u64 size;
    u64 used;
};

// Preallocate base root of application memory.
// All other memory operations will be done using this block.
void prealloc_root();
void free_root();

void  prealloc(Memory_Storage* storage, u64 size);
void* alloc(Memory_Storage* storage, u64 size);
void  free(Memory_Storage* storage, u64 size);
void  free_all(Memory_Storage* storage);
#define alloc_struct(storage, t)   (t*)alloc(storage, sizeof(t))
#define alloc_array(storage, n, t) (t*)alloc(storage, sizeof(t) * (n))
#define alloc_buffer(storage, n)   (u8*)alloc(storage, sizeof(u8) * (n))
#define free_struct(storage, t)    free(storage, sizeof(t))
#define free_array(storage, n, t)  free(storage, sizeof(t) * (n))
#define free_buffer(storage, n)    free(storage, sizeof(u8) * (n))

// Each block of functions does the same operations but for different memory blocks.
// These memory blocks enforce understanding of application requirements and therefore
// allocations should be done with keep in mind that its not possible to free data
// if another allocation was done on top of previous one. Its not necessary true for
// frame block as its cleared every frame.
// - Persistent block has application lifetime and is meant for
//   high-level abstractions like entities storage, asset registry etc.
// - Frame block has one application frame lifetime (cleared at the end of frame)
//   and useful for allocations within game frame.
// - Temporary block has application lifetime and can be used for arbitrary allocations.
// Memory operations within blocks are done in linear way.
// Before usage, block must be preallocated with desired size.
// Free operations within block are merely changing their internal pointer to
// topmost available memory. Free all is as simple as setting pointer to start.

void    prealloc_persistent(u64 size);
void*   alloc_persistent(u64 size);
void    free_persistent(u64 size);
void    free_all_persistent();
void    usage_persistent(u64* size, u64* used);
#define alloc_struct_persistent(t)   (t*)alloc_persistent(sizeof(t))
#define alloc_array_persistent(n, t) (t*)alloc_persistent(sizeof(t) * (n))
#define alloc_buffer_persistent(n)   (u8*)alloc_persistent(sizeof(u8) * (n))
#define free_struct_persistent(t)    free_persistent(sizeof(t))
#define free_array_persistent(n, t)  free_persistent(sizeof(t) * (n))
#define free_buffer_persistent(n)    free_persistent(sizeof(u8) * (n))

void    prealloc_frame(u64 size);
void*   alloc_frame(u64 size);
void    free_frame(u64 size);
void    free_all_frame();
void    usage_frame(u64* size, u64* used);
#define alloc_struct_frame(t)   (t*)alloc_frame(sizeof(t))
#define alloc_array_frame(n, t) (t*)alloc_frame(sizeof(t) * (n))
#define alloc_buffer_frame(n)   (u8*)alloc_frame(sizeof(u8) * (n))
#define free_struct_frame(t)    free_frame(sizeof(t))
#define free_array_frame(n, t)  free_frame(sizeof(t) * (n))
#define free_buffer_frame(n)    free_frame(sizeof(u8) * (n))

void    prealloc_temp(u64 size);
void*   alloc_temp(u64 size);
void    free_temp(u64 size);
void    free_all_temp();
void    usage_temp(u64* size, u64* used);
#define alloc_struct_temp(t)   (t*)alloc_temp(sizeof(t))
#define alloc_array_temp(n, t) (t*)alloc_temp(sizeof(t) * (n))
#define alloc_buffer_temp(n)   (u8*)alloc_temp(sizeof(u8) * (n))
#define free_struct_temp(t)    free_temp(sizeof(t))
#define free_array_temp(n, t)  free_temp(sizeof(t) * (n))
#define free_buffer_temp(n)    free_temp(sizeof(u8) * (n))

// @Todo: arbitrary allocations like malloc/free.
//void* alloc_arb(u64 size);
//void  free_arb(void* ptr);
