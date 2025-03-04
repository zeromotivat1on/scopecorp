#pragma once

inline constexpr u64 pers_memory_size  = MB(64);
inline constexpr u64 frame_memory_size = MB(16);
inline constexpr u64 temp_memory_size  = MB(64);

struct Memory_Storage {
	u8 *data;
	u64 size;
	u64 used;
};

inline Memory_Storage *pers  = null;
inline Memory_Storage *frame = null;
inline Memory_Storage *temp  = null;

// Preallocate application memory.
void *allocate_core();
void  release_core(void *memory);

void  init(Memory_Storage *storage, u64 size, void* memory);
void *push(Memory_Storage *storage, u64 size);
void  pop(Memory_Storage *storage, u64 size);
void  clear(Memory_Storage *storage);
#define push_struct(storage, t)   (t *)push(storage, sizeof(t))
#define push_array(storage, n, t) (t *)push(storage, sizeof(t) * (n))
#define pop_struct(storage, t)    pop(storage, sizeof(t))
#define pop_array(storage, n, t)  pop(storage, sizeof(t) * (n))

// Allocations in memory storages are done in linear way, therefore its enforce
// understanding of application memory requirements and lifetimes.
// - Persistent block has application lifetime and is meant for
//   high-level abstractions like entities storage, asset registry etc.
// - Frame block has one application frame lifetime (cleared at the end of frame)
//   and useful for allocations within game frame.
// - Temporary block has application lifetime and can be used for arbitrary allocations.
