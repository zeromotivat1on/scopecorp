#pragma once

// Fixed capacity sparse array allocated in persistent storage.
// Stores arrays of sparse and dense indices, so deleting elements from sparse array
// do not cause shift, indices are updated instead. Items stored densely as well.
template<typename T>
struct Sparse {
    T   *items    = null;
    s32 *dense    = null;
    s32 *sparse   = null;
    s32  count    = 0;
    s32  capacity = 0;
    
    T *begin() { return items; }
    T *end()   { return items + count; }
    
    const T *begin() const { return items; }
    const T *end()   const { return items + count; }

    T &operator[](s32 index) {
        Assert(index >= 0);
        Assert(index < count);
        Assert(sparse[index] != INDEX_NONE);
        return items[sparse[index]];
    }

    const T &operator[](s32 index) const {
        Assert(index >= 0);
        Assert(index < count);
        Assert(sparse[index] != INDEX_NONE);
        return items[sparse[index]];
    }
};

template<typename T>
void sparse_reserve(Arena &a, Sparse<T> &s, s32 n) {
    if (s.items) {
        warn("Attempt to reserve already reserved sparse 0x%X", &s);
        return;
    }

    s.items = arena_push_array(a, n, T);
    if (s.items == null) {
        return;
    }
    
    s.dense = arena_push_array(a, n, s32);
    if (s.dense == null) {
        arena_pop_array(a, n, T);
        return;
    }
    
    s.sparse = arena_push_array(a, n, s32);
    if (s.sparse == null) {
        arena_pop_array(a, n, s32);
        arena_pop_array(a, n, T);
        return;
    }
    
    s.capacity = n;

    // Sparse indices go immediately after dense.
    mem_set(s.dense, 0xFF, 2 * n * sizeof(s32));
}

template<typename T>
T *sparse_find(const Sparse<T> &s, s32 i) {
    if (i < 0 || i >= s.capacity) return null;
        
    const s32 si = s.sparse[i];
    if (si == INDEX_NONE) return null;
        
    return &s.items[si];
}

template<typename T>
s32 sparse_insert(Sparse<T> &s, s32 i, const T &t) {
    if (i < 0 || i >= s.capacity) return INDEX_NONE;
    if (s.count >= s.capacity)    return INDEX_NONE;
    if (sparse_find(s, i))        return INDEX_NONE;

    s.sparse[i] = s.count;
    s.dense[s.count] = i;
    s.items[s.count] = t;

    s.count += 1;
    return i;
}

template<typename T>
s32 sparse_push(Sparse<T> &s, const T &t) {
    for (s32 i = 0; i < s.capacity; ++i)
        if (s.sparse[i] == INDEX_NONE)
            return sparse_insert(s, i, t);
        
    return INDEX_NONE;
}

template<typename T>
s32 sparse_push(Sparse<T> &s) {
    return sparse_push(s, T{});
}

template<typename T>
s32 sparse_remove(Sparse<T> &s, s32 i) {
    if (i < 0 || i >= s.capacity)  return INDEX_NONE;
    if (sparse_find(s, i) == null) return INDEX_NONE;
    
    s.count -= 1;
    
    s.sparse[s.dense [s.count]] = s.sparse[i];
    s.dense [s.sparse[i]]       = s.dense[s.count];
    
    s.items [s.sparse[i]] = s.items[s.count];
    s.sparse[i] = INDEX_NONE;
    
    return i;
}

/*
template<typename T>
T *find(const Sparse<T> &array, s32 index) {
    if (index < 0 || index >= array.capacity) return null;
        
    const s32 item_index = array.sparse[index];
    if (item_index == INDEX_NONE) return null;
        
    return array.items + item_index;
}

template<typename T>
T *find_or_add_default(const Sparse<T> &array, s32 index) {
    if (index < 0 || index >= array.capacity) return null;
        
    s32 item_index = array.sparse[index];
    if (item_index == INDEX_NONE) {
        item_index = array.sparse[add_default(index)];
    }
        
    return items + item_index;
}

template<typename T>
s32 add(Sparse<T> &array, const T &item) {
    for (s32 i = 0; i < array.capacity; ++i)
        if (array.sparse[i] == INDEX_NONE)
            return add(array, i, item);
        
    return INDEX_NONE;
}

template<typename T>
s32 add_default(Sparse<T> &array) {
    for (s32 i = 0; i < array.capacity; ++i)
        if (array.sparse[i] == INDEX_NONE)
            return add_default(array, i);
        
    return INDEX_NONE;
}

template<typename T>
s32 add(Sparse<T> &array, s32 index, const T &item) {
    Assert(array.count < array.capacity);

    if (find(array, index)) return INDEX_NONE;

    array.sparse[index] = array.count;
    array.dense[array.count] = index;
    array.items[array.count] = item;

    array.count += 1;

    return index;
}

template<typename T>
s32 add_default(Sparse<T> &array, s32 index) {
    Assert(array.count < array.capacity);

    if (find(array, index)) return INDEX_NONE;

    array.sparse[index] = array.count;
    array.dense[array.count] = index;
    array.items[array.count] = T();
        
    array.count += 1;

    return index;
}

template<typename T>
s32 remove(Sparse<T> &array, s32 index) {
    Assert(index >= 0);
    Assert(index < array.count);

    if (!find(array, index)) return INDEX_NONE;

    array.count -= 1;
        
    array.sparse[array.dense[array.count]] = array.sparse[index];
    array.dense[array.sparse[index]] = array.dense[array.count];
        
    array.items[array.sparse[index]] = array.items[array.count];
    array.sparse[index] = INDEX_NONE;
        
    return index;
}
*/
