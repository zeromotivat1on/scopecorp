#pragma once

#ifndef POOL_ALIGNMENT
#define POOL_ALIGNMENT DEFAULT_ALIGNMENT
#endif

#ifndef POOL_BUCKET_SIZE
#define POOL_BUCKET_SIZE Kilobytes(64)
#endif

struct Pool {
    u64 bucket_size = POOL_BUCKET_SIZE;
    u64 alignment   = POOL_ALIGNMENT;

    Array <u8 *> used_buckets;
    Array <u8 *> unused_buckets;
    Array <u8 *> obsolete_buckets;

    u8 *current_bucket = null;
    u8 *current_pos    = null;

    u64 bytes_left = 0;

    Allocator bucket_allocator = context.allocator;
};

inline void resize_buckets(Pool *pool, u64 bucket_size) {
    pool->bucket_size = bucket_size;

    if (pool->current_bucket) array_add(pool->obsolete_buckets, pool->current_bucket);

    For (pool->used_buckets) array_add(pool->obsolete_buckets, it);
    pool->used_buckets.count = 0;

    pool->current_bucket = null;
}

inline void cycle_new_block(Pool *pool) {
    if (pool->current_bucket) array_add(pool->used_buckets, pool->current_bucket);

    u8 *new_bucket = null;
    if (pool->unused_buckets.count) {
        new_bucket = array_pop(pool->unused_buckets);
    } else {
        Assert(pool->bucket_allocator.proc);
        new_bucket = (u8 *)pool->bucket_allocator.proc(ALLOCATE, pool->bucket_size, 0, null, pool->bucket_allocator.data);
    }

    pool->bytes_left     = pool->bucket_size;
    pool->current_pos    = new_bucket;
    pool->current_bucket = new_bucket;
}

inline void ensure_memory_exists(Pool *pool, u64 size) {
    auto new_size = pool->bucket_size;
    while (new_size < size) new_size *= 2;

    if (new_size > pool->bucket_size) resize_buckets(pool, new_size);
    cycle_new_block(pool);
}

inline void *get(Pool *pool, u64 size) {
    size = Align(size, pool->alignment);

    if (pool->bytes_left < size) ensure_memory_exists(pool, size);

    auto data = pool->current_pos;
    pool->current_pos += size;
    pool->bytes_left  -= size;

    return data;
}

inline void reset(Pool *pool) {
    if (pool->current_bucket) {
        array_add(pool->unused_buckets, pool->current_bucket);
        pool->current_bucket = null;
    }

    For (pool->used_buckets) array_add(pool->unused_buckets, it);
    pool->used_buckets.count = 0;

    For (pool->obsolete_buckets) release(it);
    pool->obsolete_buckets.count = 0;

    cycle_new_block(pool);
}

inline void release(Pool *pool) {
    reset(pool);
    For (pool->unused_buckets) release(it);
}

inline void *pool_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data) {
    Assert(allocator_data);
    auto pool = (Pool *)allocator_data;
    
    switch (mode) {
    case ALLOCATE: {
        return get(pool, size);
    }
    case RESIZE: {
        auto data = get(pool, size);
        if (!data) return null;

        if (old_memory && old_size) copy(data, old_memory, old_size);
        
        return data;
    }
    case FREE: {
        return null;
    }
    case FREE_ALL: {
        reset(pool);
        return null;
    }
    default:
        unreachable_code_path();
        return null;
    }
}
