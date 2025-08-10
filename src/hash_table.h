#pragma once

inline constexpr f32 MAX_TABLE_LOAD_FACTOR = 0.7f;

template<typename K, typename V>
struct Table {
    static constexpr f32 MAX_LOAD_FACTOR = MAX_TABLE_LOAD_FACTOR;
    
    typedef u64 (*Hash   )(const K &key);
    typedef bool(*Compare)(const K &a, const K &b);

    static inline u64  default_hash   (const K &a)             { return *(u64 *)&a; }
    static inline bool default_compare(const K &a, const K &b) { return a == b; }

    K   *keys   = null;
    V   *values = null;
    u64 *hashes = null;
    u32  count    = 0;
    u32  capacity = 0;

    Hash    hash_function    = &default_hash;
    Compare compare_function = &default_compare;

    struct Iterator;
    struct Const_Iterator;
    
    Iterator begin() { return Iterator(this, 0); }
    Iterator end()   { return Iterator(this, capacity); }

    Const_Iterator begin() const { return Const_Iterator(this, 0); }
    Const_Iterator end()   const { return Const_Iterator(this, capacity); }

    V &operator[](const K &k) {
        const u64 hash = hash_function(k);
        u32 i = hash % capacity;

        while (hashes[i] != 0) {
            const auto &tk = keys[i];
            if (hashes[i] == hash && compare_function(k, tk)) {
                return values[i];
            }

            i += 1;
            if (i >= capacity) i = 0;
        }

        keys  [i] = k;
        values[i] = V{};
        hashes[i] = hash;

        count += 1;

        return values[i];
    }

    struct Iterator {
        struct Return_Value { K &key; V &value; };
        
        Table *table = null;
        u32 index = INDEX_NONE;
      
        Iterator(Table *table, u32 index)
            : table(table), index(index) {
            advance_to_valid();
        }

        inline void advance_to_valid() {
            while (index < table->capacity && table->hashes[index] == 0) {
                index += 1;
            }
        }

        inline Iterator &operator++() {
            index += 1;
            advance_to_valid();
            return *this;
        }

        inline Iterator operator++(s32) {
            Iterator copy = *this;
            ++(*this);
            return copy;
        }

        inline bool operator==(const Iterator &other) const {
            return table == other.table && index == other.index;
        }
        
        inline bool operator!=(const Iterator &other) const {
            return !(*this == other);
        }

        inline Return_Value operator*() const {
            return { table->keys[index], table->values[index] };
        }
    };

    struct Const_Iterator {
        struct Return_Value { const K &key; const V &value; };
        
        const Table *table = null;
        u32 index               = INDEX_NONE;
      
        Const_Iterator(const Table *table, u32 index)
            : table(table), index(index) {
            advance_to_valid();
        }

        inline void advance_to_valid() {
            while (index < table->capacity && table->hashes[index] == 0) {
                index += 1;
            }
        }

        inline Const_Iterator &operator++() {
            index += 1;
            advance_to_valid();
            return *this;
        }

        inline Const_Iterator operator++(s32) {
            Const_Iterator copy = *this;
            ++(*this);
            return copy;
        }

        inline bool operator==(const Const_Iterator &other) const {
            return table == other.table && index == other.index;
        }
        
        inline bool operator!=(const Const_Iterator &other) const {
            return !(*this == other);
        }

        // Dereference returns a pair of references to key and value:
        inline Return_Value operator*() const {
            return { table->keys[index], table->values[index] };
        }
    };
};

inline u32 table_optimal_capacity(u32 n) {
    constexpr f32 scale = 2.0f - MAX_TABLE_LOAD_FACTOR;
    return (u32)(n * scale);
}

template<typename K, typename V>
void table_reserve(Arena &a, Table<K, V> &t, u32 n) {
    if (t.keys) {
        return;
    }

    t.keys = arena_push_array(a, n, K);
    if (t.keys == null) {
        return;
    }
    
    t.values = arena_push_array(a, n, V);
    if (t.values == null) {
        arena_pop_array(a, n, K);
        return;
    }
    
    t.hashes = arena_push_array(a, n, u64);
    if (t.hashes == null) {
        arena_pop_array(a, n, K);
        arena_pop_array(a, n, V);
        return;
    }
    
    t.capacity = n;
}

template<typename K, typename V>
void table_custom_hash(Table<K, V> &t, typename Table<K, V>::Hash hash_function) {
    t.hash_function = hash_function;
}

template<typename K, typename V>
void table_custom_compare(Table<K, V> &t, typename Table<K, V>::Compare compare_function) {
    t.compare_function = compare_function;
}

template<typename K, typename V>
f32 table_load_factor(const Table<K, V> &t) {
    return (f32)t.count / t.capacity;
}

template<typename K, typename V>
u64 table_bytes(const Table<K, V> &t) {
    // @Cleanup: not sure if its the mose correct way to tell allocated size.
    return t.capacity * (sizeof(K) + sizeof(V) + sizeof(u64));
}

template<typename K, typename V>
void table_reset(Table<K, V> &t) {
    t.count = 0;
    
    mem_set(t.keys,   0, t.capacity * sizeof(t.keys[0]));
    mem_set(t.values, 0, t.capacity * sizeof(t.values[0]));
    mem_set(t.hashes, 0, t.capacity * sizeof(t.hashes[0]));
}

template<typename K, typename V>
V *table_find(const Table<K, V> &t, const K &k) {
    const u64 hash = t.hash_function(k);
    u32 i = hash % t.capacity;

    while (t.hashes[i] != 0) {
        const auto &tk = t.keys[i];
        if (t.hashes[i] == hash && t.compare_function(k, tk)) {
            return &t.values[i];
        }

        i += 1;
        if (i >= t.capacity) i = 0;
    }

    return null;
}

template<typename K, typename V>
V *table_push(Table<K, V> &t, const K &k, const V &v) {
    Assert(table_load_factor(t) < t.MAX_LOAD_FACTOR);
    
    const u64 hash = t.hash_function(k);
    u32 i = hash % t.capacity;

    while (t.hashes[i] != 0) {
        i += 1;
        if (i >= t.capacity) i = 0;
    }

    t.keys  [i] = k;
    t.values[i] = v;
    t.hashes[i] = hash;

    t.count += 1;
    return &t.values[i];
}

template<typename K, typename V>
bool table_remove(Table<K, V> &t, const K &k) {
    const u64 hash = t.hash_function(key);
    u32 i = hash % t.capacity;

    while (t.hashes[i] != 0) {
        const auto &tk = t.keys[i];
        if (t.hashes[i] == hash && t.compare_function(k, tk)) {
            t.hashes[i] = 0;
            t.count -= 1;
            return true;
        }

        i += 1;
        if (i >= capacity) i = 0;
    }

    return false;
}
