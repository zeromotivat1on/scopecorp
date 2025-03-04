#pragma once

#include "memory_storage.h"
#include <string.h>

// Fixed capacity array allocated in persistent memory block.
// Stores list of free indices, so deleting elements from slot array
// do not cause shift, free index list is updated instead.
template<typename T>
struct Sparse_Array {    
    T* items     = null;
    s32* dense   = null;
    s32* sparse  = null;
    s32 count    = 0;
    s32 capacity = 0;

    Sparse_Array() = default;
    Sparse_Array(s32 capacity)
        : capacity(capacity),
          items((T*)push(pers, capacity * sizeof(T))),
          dense(push_array(pers, capacity, s32)),
          sparse(push_array(pers, capacity, s32)) {
        memset(dense,  0xFF, capacity * sizeof(s32));
        memset(sparse, 0xFF, capacity * sizeof(s32));
    }
    
    T& operator[](s32 idx) {
        assert(idx >= 0);
        assert(idx < count);
        assert(sparse[idx] != INVALID_INDEX);
        return items[sparse[idx]];
    }

    const T& operator[](s32 idx) const {
        assert(idx < count);
        assert(sparse[idx] != INVALID_INDEX);
        return items[sparse[idx]];
    }

    T* find(s32 idx) {
        assert(idx >= 0);
        assert(idx < capacity); // find has less strict rules for index
        
        const s32 item_idx = sparse[idx];
        if (item_idx == INVALID_INDEX) return null;
        
        return items + item_idx;
    }

    s32 add(const T& item) {
        for (s32 i = 0; i < count + 1; ++i)
            if (sparse[i] == INVALID_INDEX)
                return add(i, item);
        
        return INVALID_INDEX;
    }

    s32 add_default() {
        for (s32 i = 0; i < count + 1; ++i)
            if (sparse[i] == INVALID_INDEX)
                return add_default(i);
        
        return INVALID_INDEX;
    }

    s32 add(s32 idx, const T& item) {
        assert(count < capacity);

        if (find(idx)) return INVALID_INDEX;

        sparse[idx] = count;
        dense[count] = idx;
        items[count] = item;

        count++;

        return idx;
    }

    s32 add_default(s32 idx) {
        assert(count < capacity);

        if (find(idx)) return INVALID_INDEX;

        sparse[idx] = count;
        dense[count] = idx;
        items[count] = T();
        
        count++;

        return idx;
    }
    
    s32 remove(s32 idx) {
        assert(idx >= 0);
        assert(idx < count);

        if (!find(idx)) return INVALID_INDEX;

        count--;
        
        sparse[dense[count]] = sparse[idx];
        dense[sparse[idx]] = dense[count];
        
        items[sparse[idx]] = items[count];
        sparse[idx] = INVALID_INDEX;
        
        return idx;
    }
};
