#pragma once

inline constexpr f32 MAX_HASH_TABLE_LOAD_FACTOR = 0.7f;

template<typename K, typename V>
struct Hash_Table {
    typedef u64 (*Hash   )(const K &key);
    typedef bool(*Compare)(const K &a, const K &b);

    static inline u64  default_hash   (const K &a)             { return *(u64 *)&a; }
    static inline bool default_compare(const K &a, const K &b) { return a == b; }

    K   *keys   = null;
    V   *values = null;
    u64 *hashes = null;
    s32  count    = 0;
    s32  capacity = 0;

    Hash    hash_function    = &default_hash;
    Compare compare_function = &default_compare;
    
    Hash_Table() = default;
    Hash_Table(s32 capacity)
        : capacity(capacity),
          keys  (allocltn(K,   capacity)),
          values(allocltn(V,   capacity)),
          hashes(allocltn(u64, capacity)) {
        for (s32 i = 0; i < capacity; ++i) {
            hashes[i] = 0;
        }
    }

    struct Iterator;
    struct Const_Iterator;
    
    Iterator begin() { return Iterator(this, 0); }
    Iterator end()   { return Iterator(this, capacity); }

    Const_Iterator begin() const { return Const_Iterator(this, 0); }
    Const_Iterator end()   const { return Const_Iterator(this, capacity); }

    V &operator[](const K &key) const {
        const u64 hash = hash_function(key);
        s32 index = hash % capacity;

        while (hashes[index] != 0) {
            const auto &table_key = keys[index];
            if (hashes[index] == hash && compare_function(key, table_key)) {
                return values[index];
            }

            index += 1;
            if (index >= capacity) index = 0;
        }

        keys  [index] = key;
        values[index] = V();
        hashes[index] = hash;
        
        return values[index];
    }

    struct Iterator {
        struct Return_Value { K &key; V &value; };
        
        Hash_Table *table = null;
        s32 index         = INVALID_INDEX;
      
        Iterator(Hash_Table *table, s32 index)
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

        // Dereference returns a pair of references to key and value:
        inline Return_Value operator*() const {
            return { table->keys[index], table->values[index] };
        }
    };

    struct Const_Iterator {
        struct Return_Value { const K &key; const V &value; };
        
        const Hash_Table *table = null;
        s32 index               = INVALID_INDEX;
      
        Const_Iterator(const Hash_Table *table, s32 index)
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

template<typename K, typename V>
f32 load_factor(const Hash_Table<K, V> &table) {
    return (f32)table.count / table.capacity;
}

template<typename K, typename V>
V *find(const Hash_Table<K, V> &table, const K &key) {
    const u64 hash = table.hash_function(key);
    s32 index = hash % table.capacity;

    while (table.hashes[index] != 0) {
        const auto &table_key = table.keys[index];
        if (table.hashes[index] == hash && table.compare_function(key, table_key)) {
            return table.values + index;
        }

        index += 1;
        if (index >= table.capacity) index = 0;
    }

    return null;
}

template<typename K, typename V>
V *add(Hash_Table<K, V> &table, const K &key, const V &value) {
    Assert(load_factor(table) < MAX_HASH_TABLE_LOAD_FACTOR);
        
    const u64 hash = table.hash_function(key);
    s32 index = hash % table.capacity;

    while (table.hashes[index] != 0) {
        index += 1;
        if (index >= table.capacity) index = 0;
    }

    table.keys  [index] = key;
    table.values[index] = value;
    table.hashes[index] = hash;

    table.count += 1;
    return table.values + index;
}

template<typename K, typename V>
bool remove(Hash_Table<K, V> &table, const K &key) {
    const u64 hash = table.hash_function(key);
    s32 index = hash % table.capacity;

    while (table.hashes[index] != 0) {
        const auto &table_key = keys[index];
        if (table.hashes[index] == hash && table.compare_function(key, table_key)) {
            table.hashes[index] = 0;
            count--;
            return true;
        }

        index += 1;
        if (index >= capacity) index = 0;
    }

    return false;
}
