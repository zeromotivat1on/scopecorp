#pragma once

// Fixed capacity array allocated in persistent memory block.
// Stores list of sparse and dense indices, so deleting elements from sparse array
// do not cause shift, indices are updated instead. Items stored densely as well.
template<typename T>
struct Sparse_Array {    
    T   *items    = null;
    s32 *dense    = null;
    s32 *sparse   = null;
    s32  count    = 0;
    s32  capacity = 0;

    Sparse_Array() = default;
    Sparse_Array(s32 capacity)
        : capacity(capacity),
          items (allocltn(T,   capacity)),
          dense (allocltn(s32, capacity)),
          sparse(allocltn(s32, capacity)) {
        for (s32 i = 0; i < capacity; ++i) {
            dense[i] = sparse[i] = INVALID_INDEX;
        }
    }
    
    T *begin() { return items; }
    T *end()   { return items + count; }
    
    const T *begin() const { return items; }
    const T *end()   const { return items + count; }

    T &operator[](s32 index) {
        Assert(index >= 0);
        Assert(index < count);
        Assert(sparse[index] != INVALID_INDEX);
        return items[sparse[index]];
    }

    const T &operator[](s32 index) const {
        Assert(index < count);
        Assert(sparse[index] != INVALID_INDEX);
        return items[sparse[index]];
    }

    T *find(s32 index) {
        Assert(index >= 0);
        Assert(index < capacity);
        
        const s32 item_index = sparse[index];
        if (item_index == INVALID_INDEX) return null;
        
        return items + item_index;
    }

    T *find_or_add_default(s32 index) {
        Assert(index >= 0);
        Assert(index < capacity);
        
        s32 item_index = sparse[index];
        if (item_index == INVALID_INDEX) {
            item_index = sparse[add_default(index)];
        }
        
        return items + item_index;
    }
    
    s32 add(const T &item) {
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

    s32 add(s32 index, const T &item) {
        Assert(count < capacity);

        if (find(index)) return INVALID_INDEX;

        sparse[index] = count;
        dense[count] = index;
        items[count] = item;

        count++;

        return index;
    }

    s32 add_default(s32 index) {
        Assert(count < capacity);

        if (find(index)) return INVALID_INDEX;

        sparse[index] = count;
        dense[count] = index;
        items[count] = T();
        
        count++;

        return index;
    }
    
    s32 remove(s32 index) {
        Assert(index >= 0);
        Assert(index < count);

        if (!find(index)) return INVALID_INDEX;

        count--;
        
        sparse[dense[count]] = sparse[index];
        dense[sparse[index]] = dense[count];
        
        items[sparse[index]] = items[count];
        sparse[index] = INVALID_INDEX;
        
        return index;
    }
};
