#pragma once

#include <stdint.h>
#include <assert.h>

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

#define null nullptr

#define INVALID_INDEX -1

#define KB(n) (n * 1024ULL)
#define MB(n) (KB(n) * 1024ULL)
#define GB(n) (MB(n) * 1024ULL)
#define TB(n) (GB(n) * 1024ULL)

#if DEBUG
inline const char* build_type_name = "DEBUG";
#elif RELEASE
inline const char* build_type_name = "RELEASE";
#else
inline const char* build_type_name = "UNKNOWN";
#endif

// Preallocate base root of application memory.
// All other memory operations will be done using this block.
void prealloc_root();
void free_root();

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
// topmost available memory. Free all is as simple as setting pointer to zero.

void    prealloc_persistent(u64 size);
void*   alloc_persistent(u64 size);
void    free_persistent(u64 size);
void    free_all_persistent();
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
#define alloc_struct_temp(t)   (t*)alloc_temp(sizeof(t))
#define alloc_array_temp(n, t) (t*)alloc_temp(sizeof(t) * (n))
#define alloc_buffer_temp(n)   (u8*)alloc_temp(sizeof(u8) * (n))
#define free_struct_temp(t)    free_temp(sizeof(t))
#define free_array_temp(n, t)  free_temp(sizeof(t) * (n))
#define free_buffer_temp(n)    free_temp(sizeof(u8) * (n))

// @Todo: arbitrary allocations like malloc/free.
//void* alloc_arb(u64 size);
//void  free_arb(void* ptr);

void log(const char* format, ...);

// Globals
// @Cleanup: is it really worth to have them here?
inline struct Window* window = null;
inline struct Font_Render_Context* font_render_ctx = null;
inline struct World* world = null;
