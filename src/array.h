#pragma once

#include <malloc.h>

// @Cleanup: array cannot auto-grow, maybe leave it as feature.

// Simple heap allocated array of even-sized items.
// Allocations are done on memory level, so items are not constructed.
template<typename T>
struct Array
{
    T* items;
    s32 count;
    s32 capacity;

    Array()
    {
        *this = {0};
    }
    
    Array(s32 new_capacity)
    {
        items = malloc(capacity * sizeof(T));
        count = 0;
        capacity = new_capacity;
    }

    ~Array()
    {
        if (items) free(items);
    }
    
    T& operator[](s32 idx)
    {
        assert(idx < count);
        return items[idx];
    }

    const T& operator[](s32 idx) const
    {
        assert(idx < count);
        return items[idx];
    }

    void realloc(s32 new_capacity)
    {
        items = realloc(items, new_capacity * sizeof(T));
        capacity = new_capacity;
    }
    
    s32 add(const T& item)
    {
        if (count == capacity) return INVALID_INDEX;
        items[count++] = item;
        return count - 1;
    }

    s32 insert(s32 idx, const T& item)
    {
        if (idx > count) return INVALID_INDEX;
        if (count == capacity) return INVALID_INDEX;

        count++;
        for (s32 i = count; i > idx; --i)
            items[i] = items[i - 1];

        items[idx] = item;
        return idx;
    }

    s32 remove(s32 idx)
    {
        if (idx >= count) return INVALID_INDEX;    
        for (s32 i = idx; i < count; ++i)
            items[i] = items[i + 1];
        count--;
        return idx;
    }

    s32 remove_swap_last(s32 idx)
    {
        if (idx >= count) return INVALID_INDEX;
        swap(items[idx], items[count - 1]);
        count--;
        return idx;
    }
};
