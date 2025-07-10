#pragma once

// Fixed capacity sparse array allocated in linear storage.
// Stores arrays of sparse and dense indices, so deleting elements from sparse array
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
        Assert(index >= 0);
        Assert(index < count);
        Assert(sparse[index] != INVALID_INDEX);
        return items[sparse[index]];
    }
};

template<typename T>
T *find(const Sparse_Array<T> &array, s32 index) {
    if (index < 0 || index >= array.capacity) return null;
        
    const s32 item_index = array.sparse[index];
    if (item_index == INVALID_INDEX) return null;
        
    return array.items + item_index;
}

template<typename T>
T *find_or_add_default(const Sparse_Array<T> &array, s32 index) {
    if (index < 0 || index >= array.capacity) return null;
        
    s32 item_index = array.sparse[index];
    if (item_index == INVALID_INDEX) {
        item_index = array.sparse[add_default(index)];
    }
        
    return items + item_index;
}

template<typename T>
s32 add(Sparse_Array<T> &array, const T &item) {
    for (s32 i = 0; i < array.capacity; ++i)
        if (array.sparse[i] == INVALID_INDEX)
            return add(array, i, item);
        
    return INVALID_INDEX;
}

template<typename T>
s32 add_default(Sparse_Array<T> &array) {
    for (s32 i = 0; i < array.capacity; ++i)
        if (array.sparse[i] == INVALID_INDEX)
            return add_default(array, i);
        
    return INVALID_INDEX;
}

template<typename T>
s32 add(Sparse_Array<T> &array, s32 index, const T &item) {
    Assert(array.count < array.capacity);

    if (find(array, index)) return INVALID_INDEX;

    array.sparse[index] = count;
    array.dense[count] = index;
    array.items[count] = item;

    array.count += 1;

    return index;
}

template<typename T>
s32 add_default(Sparse_Array<T> &array, s32 index) {
    Assert(array.count < array.capacity);

    if (find(array, index)) return INVALID_INDEX;

    array.sparse[index] = array.count;
    array.dense[array.count] = index;
    array.items[array.count] = T();
        
    array.count += 1;

    return index;
}

template<typename T>
s32 remove(Sparse_Array<T> &array, s32 index) {
    Assert(index >= 0);
    Assert(index < array.count);

    if (!find(array, index)) return INVALID_INDEX;

    array.count -= 1;
        
    array.sparse[array.dense[array.count]] = array.sparse[index];
    array.dense[array.sparse[index]] = array.dense[array.count];
        
    array.items[array.sparse[index]] = array.items[array.count];
    array.sparse[index] = INVALID_INDEX;
        
    return index;
}
