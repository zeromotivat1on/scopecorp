#pragma once

struct u128 {
    u64 buckets[2];
};

struct u256 {
    u64 buckets[4];
};

struct u512 {
    u64 buckets[8];
};

inline void set(u64 *buckets, u16 pos) {
    buckets[pos / 64] |= (1ull << (pos % 64));
}

inline void clear(u64 *buckets, u16 pos) {
    buckets[pos / 64] &= ~(1ull << (pos % 64));
}

inline void toggle(u64 *buckets, u16 pos) {
    buckets[pos / 64] ^= (1ull << (pos % 64));
}

inline bool check(const u64 *buckets, u16 pos) {
    return (buckets[pos / 64] & (1ull << (pos % 64))) != 0;
}

inline void set_whole(u64 *buckets, u8 count, u8 start = 0) {
    set_bytes(buckets + start, 0xFF, count * sizeof(u64));
}

inline void clear_whole(u64 *buckets, u8 count, u8 start = 0) {
    set_bytes(buckets + start, 0, count * sizeof(u64));
}

inline void and(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] & rbuckets[i];
    }
}

inline void or(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] | rbuckets[i];
    }
}

inline void xor(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] ^ rbuckets[i];
    }
}

inline void not(u64 *res_buckets, const u64 *buckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = ~buckets[i];
    }
}
