#pragma once

template <typename T, u32 N>
struct C_Array {
    T items[N] = {};
    u32 count = 0;
    const u32 capacity = N;
    
    T *begin() { return items; }
    T *end()   { return items + count; }
    
    const T *begin() const { return items; }
    const T *end()   const { return items + count; }

    T &operator[](u32 index) {
        Assert(index < capacity);
        return items[index];
    }

    const T &operator[](u32 index) const {
        Assert(index < capacity);
        return items[index];
    }
};

template <typename T, u32 N>
T &add(C_Array<T, N> &array, const T& item) {
    Assert(array.count < array.capacity);
    T &v = array.items[array.count] = item;
    array.count += 1;
    return v;
}
