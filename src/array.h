#pragma once

template<typename T>
struct Array {
    T* items     = null;
    u32 count    = 0;
    u32 capacity = 0;

    Array() = default;
    Array(u32 capacity)
        : items(allocltn(T, capacity)), capacity(capacity) {}
        
    T *begin() { return items; }
    T *end()   { return items + count; }
    
    const T *begin() const { return items; }
    const T *end()   const { return items + count; }
    
    T& operator[](u32 index) {
        Assert(index < count);
        return items[index];
    }

    const T& operator[](u32 index) const {
        Assert(index < count);
        return items[index];
    }
};

template<typename T>
T &add(Array<T> &array, const T& item) {
    Assert(array.count < array.capacity);
    T &v = array.items[array.count] = item;
    array.count += 1;
    return v;
}

template<typename T>
T &insert(Array<T> &array, u32 index, const T& item) {
    Assert(array.count < array.capacity);
    Assert(index < array.count);

    array.count += 1;
    for (u32 i = array.count; i > index; --i)
        array.items[i] = array.items[i - 1];

    T &v = array.items[index] = item;
    return v;
}

template<typename T>
void remove(Array<T> &array, u32 index) {
    Assert(index < array.count);

    for (s32 i = index; i < array.count; ++i) {
        array.items[i] = array.items[i + 1];
    }
    
    count--;
}

template<typename T>
void remove_swap_last(Array<T> &array, u32 index) {
    Assert(index < array.count);

    T t = array.items[index];
    array.items[index] = array.items[count - 1];
    array.items[count - 1] = t;

    array.count -= 1;
}
