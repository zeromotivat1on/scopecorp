#pragma once

#include "memory.h"

#ifndef VIRTUAL_ARENA_ALIGNMENT
#define VIRTUAL_ARENA_ALIGNMENT DEFAULT_ALIGNMENT
#endif

#ifndef VIRTUAL_ARENA_RESERVE_SIZE
#define VIRTUAL_ARENA_RESERVE_SIZE Megabytes(64)
#endif

struct Virtual_Arena {
    u64 reserve_alignment = get_allocation_granularity();
    u64 reserve_size      = VIRTUAL_ARENA_RESERVE_SIZE;
    u64 commit_alignment  = get_page_size();
    u64 alignment         = VIRTUAL_ARENA_ALIGNMENT;
    
    void *base     = null;
    u64   reserved = 0;
    u64   commited = 0;
    u64   used     = 0;
};

inline bool reserve(Virtual_Arena *arena, u64 size) {
    Assert(!arena->base);
    
    size = Align(size, arena->reserve_alignment);
    arena->base = virtual_reserve(null, size);

    if (!arena->base) {
        log(LOG_ERROR, "Failed to reserve virtual memory of size %llu bytes for arena 0x%X", size, arena);
        return false;
    }

    arena->reserved = size;
    return true;
}

inline bool commit(Virtual_Arena *arena, u64 size) {
    Assert(arena->base);

    size = arena->commited + size;
    size = Align(size, arena->commit_alignment) - arena->commited;

    if (arena->commited + size > arena->reserved) {
        log(LOG_ERROR, "Commit size of %llu bytes will result in overflow of reserved size %llu bytes in virtual arena 0x%X, commited %llu bytes", size, arena->reserved, arena, arena->commited);
        return false;
    }
    
    auto p = (u8 *)arena->base + arena->commited;
    if (!virtual_commit(p, size)) {
        log(LOG_ERROR, "Failed to commit %llu bytes for virtual arena 0x%X", size, arena);
        return false;
    }
    
    arena->commited += size;
    return true;
}

inline void *get(Virtual_Arena *arena, u64 size) {
    if (!arena->base) if (!reserve(arena, arena->reserve_size)) return null;

    size = Align(size, arena->alignment);

    if (arena->used + size > arena->reserved) {
        log(LOG_ERROR, "Alloc size of %llu bytes will result in overflow of reserved size %llu bytes in virtual arena 0x%X, used %llu bytes", size, arena->reserved, arena, arena->used);
        return null;
    }

    if (arena->used + size > arena->commited) {
        auto commit_size = arena->used + size - arena->commited;
        if (!commit(arena, commit_size)) return null;
    }

    auto data = (u8 *)arena->base + arena->used;
    arena->used += size;
    
    return data;
}

inline void reset(Virtual_Arena *arena) {
    if (!virtual_decommit(arena->base, arena->commited)) {
        log(LOG_ERROR, "Failed to decommit virtual memory 0x%X of size %llu bytes in virtual arena 0x%X", arena->base, arena->commited, arena);
        return;
    }

    arena->commited = 0;
    arena->used     = 0;
}

inline void release(Virtual_Arena *arena) {
    reset(arena);

    if (!virtual_release(arena->base)) {
        log(LOG_ERROR, "Failed to release virtual memory 0x%X of size %llu bytes in virtual arena 0x%X", arena->base, arena->reserved, arena);
        return;
    }

    arena->base     = null;
    arena->reserved = 0;
    arena->commited = 0;
    arena->used     = 0;
}

inline void *virtual_arena_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data) {
    Assert(allocator_data);
    auto arena = (Virtual_Arena *)allocator_data;
    
    switch (mode) {
    case ALLOCATE: {
        return get(arena, size);
    }
    case RESIZE: {
        auto data = get(arena, size);
        if (!data) return null;

        if (old_memory && old_size) copy(data, old_memory, old_size);
        
        return data;
    }
    case FREE: {
        return null;
    }
    case FREE_ALL: {
        reset(arena);
        return null;
    }
    default:
        unreachable_code_path();
        return null;
    }
}
