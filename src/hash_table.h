#pragma once

#include "hash.h"

template <typename T> u64 table_hash_proc(const T &v)      { return *(u64 *)&v; }
template <>           u64 table_hash_proc(const String &s) { return hash_fnv(s); }

template<typename K, typename V>
struct Table {
    static constexpr u32 INITIAL_CAPACITY = 32;    
    static constexpr f32 MAX_LOAD_FACTOR  = 0.7f;
    static constexpr u64 EMPTY_HASH       = 0;
    static constexpr u64 REMOVED_HASH     = 1;
    
    typedef u64 (*Hash   )(const K &key);
    typedef bool(*Compare)(const K &a, const K &b);

    struct Entry {
        K key;
        V value;
        u64 hash = 0;
    };

    Allocator allocator = context.allocator;

    Entry *entries  = null;
    u32    count    = 0;
    u32    capacity = 0;

    Hash    hash_function    = null;
    Compare compare_function = null;
    
    struct Iterator;
    
    Iterator begin() { return Iterator(*this, 0); }
    Iterator end()   { return Iterator(*this, capacity); }

    const Iterator begin() const { return Iterator(*this, 0); }
    const Iterator end()   const { return Iterator(*this, capacity); }

    V &operator[](const K &key) { return table_add(*this, key); }

    u64 hash(const K &key) const {
        if (hash_function) return hash_function(key);
        return table_hash_proc(key);
    }

    bool compare(const K &a, const K &b) const {
        if (compare_function) return compare_function(a, b);
        return a == b;
    }
    
    struct Iterator {
        const Table &table;
        u32 index = INDEX_NONE;
      
        Iterator(const Table &table, u32 index)
            : table(table), index(index) { advance_to_valid(); }

        inline void advance_to_valid() {
            while (true) {
                if (index >= table.capacity) return;

                const auto &entry = table.entries[index];
                if (entry.hash != EMPTY_HASH && entry.hash != REMOVED_HASH) return;
                
                index += 1;
            }
        }

        inline Iterator &operator++()   { index += 1; advance_to_valid(); return *this; }
        inline Iterator operator++(s32) { Iterator it = *this; ++(*this); return it; }

        inline bool operator==(const Iterator &other) const { return &table == &other.table && index == other.index; }
        inline bool operator!=(const Iterator &other) const { return !(*this == other); }

        inline Entry &operator*() const { return table.entries[index]; }
    };
};

template<typename K, typename V>
void table_set_hash(Table<K, V> &table, typename Table<K, V>::Hash hash) {
    table.hash_function = hash;
}

template<typename K, typename V>
void table_set_compare(Table<K, V> &table, typename Table<K, V>::Compare compare) {
    table.compare_function = compare;
}

template<typename K, typename V>
u32 table_desired_bucket(const Table<K, V> &table, u64 hash) {
    Assert(hash != table.EMPTY_HASH);
    return hash % table.capacity;
}

template<typename K, typename V>
f32 table_load_factor(const Table<K, V> &table) {
    if (table.count == 0 || table.capacity == 0) return 0;
    return (f32)table.count / table.capacity;
}

template<typename K, typename V>
void table_clear(Table<K, V> &table) {
    table.count = 0;
    set(table.entries, 0, table.capacity * sizeof(table.entries[0]));
}

template<typename K, typename V>
void table_realloc(Table<K, V> &table, u32 new_capacity) {
    typedef Table<K, V>::Entry Table_Entry;

    new_capacity += (u32)Ceil((1.0f - table.MAX_LOAD_FACTOR) * new_capacity);
    if (new_capacity <= table.capacity) return;

    const auto old_count   = table.count;
    const auto old_entries = table.entries;
    
    table.count    = 0;
    table.capacity = new_capacity;
    table.entries  = (Table_Entry *)table.allocator.proc(ALLOCATE, new_capacity * sizeof(Table_Entry), 0, null, table.allocator.data);
    
    u32 rehashed_count = 0;
    for (u32 i = 0; i < table.capacity; ++i) {
        if (rehashed_count == old_count) break;
        
        const auto &old_entry = old_entries[i];
        if (old_entry.hash != table.EMPTY_HASH && old_entry.hash != table.REMOVED_HASH) {
            table_add(table, old_entry.key, old_entry.value);
            rehashed_count += 1;
        }
    }
}

template<typename K, typename V>
V *table_find(const Table<K, V> &table, const K &key) {
    if (table.count == 0 || table.capacity == 0) return null;
    
    const u64 hash = table.hash(key) + 1;
    u32 bucket = table_desired_bucket(table, hash);

    while (true) {
        auto &entry = table.entries[bucket];
        if (entry.hash == table.EMPTY_HASH) return null;

        if (entry.hash == hash && table.compare(entry.key, key)) {
            return &entry.value;
        }
        
        bucket += 1;
        if (bucket >= table.capacity) bucket = 0;
    }

    return null;
}

template<typename K, typename V>
V &table_add(Table<K, V> &table, const K &key) {
    if (table.capacity == 0 || table_load_factor(table) >= table.MAX_LOAD_FACTOR) {
        const u32 new_capacity = Max(table.capacity * 2 + 1, table.INITIAL_CAPACITY);
        table_realloc(table, new_capacity);
    }
        
    const u64 hash = table.hash(key) + 1;
    u32 bucket = table_desired_bucket(table, hash);
    u32 first_tombstone = U32_MAX;
    
    while (true) {
        auto &entry = table.entries[bucket];

        // Cache first removed bucket (tombstone) if any to update it
        // instead of active bucket to preserve shorter prob chains.
        if (entry.hash == table.REMOVED_HASH) {
            if (first_tombstone == U32_MAX) {
                first_tombstone = bucket;
            }
        }
        
        if (entry.hash == table.EMPTY_HASH) {
            table.count += 1;

            const u32 target_bucket = (first_tombstone == U32_MAX) ? bucket : first_tombstone;
            auto &target = table.entries[target_bucket];
            
            target.key  = key;
            target.hash = hash;
            new (&target.value) V; // default construct for the very first add
            
            return target.value;
        }

        // Check for existing entry.
        if (entry.hash == hash && table.compare(entry.key, key)) {
            return entry.value;
        }
        
        bucket += 1;
        if (bucket >= table.capacity) bucket = 0;
    }
}

template<typename K, typename V>
V &table_add(Table<K, V> &table, const K &key, const V &value) {
    auto v = new (&table_add(table, key)) V {value};
    return *v;
}

template<typename K, typename V>
bool table_remove(Table<K, V> &table, const K &key) {
    const u64 hash = table.hash(key) + 1;
    u32 bucket = table_desired_bucket(table, hash);
    
    while (true) {
        auto &entry = table.entries[bucket];
        if (entry.hash == table.EMPTY_HASH) return false;

        if (entry.hash == hash && table.compare(entry.key, key)) {
            table.count -= 1;
            entry.hash = table.REMOVED_HASH;
            return true;
        }
        
        bucket += 1;
        if (bucket >= table.capacity) bucket = 0;
    }

    return false;
}
