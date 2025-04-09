#pragma once

inline constexpr u64 PERS_MEMORY_SIZE  = MB(64);
inline constexpr u64 FRAME_MEMORY_SIZE = MB(16);
inline constexpr u64 TEMP_MEMORY_SIZE  = MB(128);
inline constexpr u64 PANIC_MEMORY_SIZE = MB(1);

struct Memory_Storage {
	u8 *data;
	u64 size;
	u64 used;
};

inline Memory_Storage *pers  = null;
inline Memory_Storage *frame = null;
inline Memory_Storage *temp  = null;
inline Memory_Storage *panic = null;

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
//
// - Persistent block has application lifetime and is meant for
//   high-level abstractions like entities storage, asset registry etc.
//
// - Frame block has one application frame lifetime (cleared at the end of frame)
//   and useful for allocations within game frame.
//
// - Temporary block has application lifetime and can be used for arbitrary allocations.
//   You can clear it explicitely whenever its needed.
//
// - Panic block has application lifetime and is used in code that should have been
//   returned other actual data, but returned this one instead with error logged.
//   The client code may think that returned memory is valid and at some situations
//   an application may show expected behavior, but the block generally stores garbage
//   data and can be thought of as an error fallback path, that indicates a significant
//   error in code that should be fixed, but at least allows to ignore immediate crashes
//   like access violation.
