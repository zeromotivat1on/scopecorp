#pragma once

#include "assertion.h"
#include "memory_storage.h"

// Fixed capacity array allocated in persistent memory block.
template<typename T>
struct Array {
    T* items     = null;
    s32 count    = 0;
    s32 capacity = 0;

    Array() = default;
    Array(s32 capacity)
        : capacity(capacity), items((T*)push(pers, capacity * sizeof(T))) {}
    
    T& operator[](s32 idx) {
        assert(idx < count);
        return items[idx];
    }

    const T& operator[](s32 idx) const {
        assert(idx < count);
        return items[idx];
    }
    
    s32 add(const T& item) {
        assert(count < capacity);
        items[count++] = item;
        return count - 1;
    }

    s32 insert(s32 idx, const T& item) {
        assert(count < capacity);
        assert(idx < count);

        count++;
        for (s32 i = count; i > idx; --i)
            items[i] = items[i - 1];

        items[idx] = item;
        return idx;
    }

    s32 remove(s32 idx) {
        assert(idx < count);

        for (s32 i = idx; i < count; ++i)
            items[i] = items[i + 1];
        
        count--;
        return idx;
    }

    s32 remove_swap_last(s32 idx) {
        assert(idx < count);
        swap(items[idx], items[count - 1]);
        count--;
        return idx;
    }
};
