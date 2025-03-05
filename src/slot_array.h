#pragma once

#include "assertion.h"
#include "memory_storage.h"

// Fixed capacity array allocated in persistent memory block.
// Stores list of free indices, so deleting elements from slot array
// do not cause shift, free index list is updated instead.
template<typename T>
struct Slot_Array {
    T*  items    = null;
    s32 count    = 0; // filled slot count, last filled slot index can be greater than this
    s32 capacity = 0;

    s32* free_indices     = null;
    s32  free_index_count = 0;
    
    Slot_Array() = default;
    Slot_Array(s32 capacity)
        : capacity(capacity),
          items((T*)push(pers, capacity * sizeof(T))),
          free_indices(push_array(pers, capacity, s32)) {}
    
    T& operator[](s32 idx) {
        assert(idx >= 0);
        assert(idx < count + free_index_count); // take into account free slots
        return items[idx];
    }

    const T& operator[](s32 idx) const {
        assert(idx >= 0);
        assert(idx < count + free_index_count); // take into account free slots
        return items[idx];
    }
    
    s32 add(const T& item) {
        assert(count < capacity);

        if (free_index_count > 0) {
            free_index_count--;
            const s32 idx = free_indices[free_index_count];
            items[idx] = item;
            return idx;
        }

        const s32 idx = count;
        items[idx] = item;
        count++;
        
        return idx;
    }

    s32 remove(s32 idx) {
        assert(idx >= 0);
        assert(idx < count + free_index_count);
        
        free_indices[free_index_count] = idx;
        free_index_count++;
        
        count--;
        return idx;
    }
};
