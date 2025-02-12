#pragma once

#include <string.h>

struct Arena
{
    u8* base;
    u64 size;
    u64 used;
};

#define push_struct(arena, type)       (type*)push(arena, sizeof(type))
#define push_array(arena, count, type) (type*)push(arena, sizeof(type) * (count))

inline void init_arena(Arena* arena, void* base, u64 size)
{
    arena->base = (u8*)base;
    arena->size = size;
    arena->used = 0;
}

inline void clear(Arena* arena)
{
    arena->used = 0;
}

inline u8* push(Arena* arena, u64 size)
{
    assert(arena->used + size <= arena->size);
    u8* data = arena->base + arena->used;
    arena->used += size;
    return data;
}

inline u8* push_zero(Arena* arena, u64 size)
{
    u8* data = push(arena, size);
    // @Cleanup: make own memset or replace it with loop to remove include?
    memset(data, 0, size);
    return data;
}

inline void pop(Arena* arena, u64 size)
{
    assert(arena->used >= size);
    arena->used -= size;
}

inline Arena create_arena(void* base, u64 size)
{
    Arena arena;
    init_arena(&arena, base, size);
    return arena;
}

inline Arena subarena(Arena* arena, u64 size)
{
    return create_arena(push(arena, size), size);
}
