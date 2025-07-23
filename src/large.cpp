#include "large.h"

void set(u64 *buckets, u16 pos) {
    buckets[pos / 64] |= (1ull << (pos % 64));
}

void clear(u64 *buckets, u16 pos) {
    buckets[pos / 64] &= ~(1ull << (pos % 64));
}

void toggle(u64 *buckets, u16 pos) {
    buckets[pos / 64] ^= (1ull << (pos % 64));
}

bool check(const u64 *buckets, u16 pos) {
    return (buckets[pos / 64] & (1ull << (pos % 64))) != 0;
}

void set_whole(u64 *buckets, u8 count, u8 start) {
    mem_set(buckets + start, 0xFF, count * sizeof(u64));
}

void clear_whole(u64 *buckets, u8 count, u8 start) {
    mem_set(buckets + start, 0, count * sizeof(u64));
}

void and(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] & rbuckets[i];
    }
}

void or(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] | rbuckets[i];
    }
}

void xor(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] ^ rbuckets[i];
    }
}

void not(u64 *res_buckets, const u64 *buckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = ~buckets[i];
    }
}

u256 operator&(const u256 &a, const u256 &b) {
    u256 res;
    and(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator|(const u256 &a, const u256 &b) {
    u256 res;
    or(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator^(const u256 &a, const u256 &b) {
    u256 res;
    xor(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator~(const u256 &a) {
    u256 res;
    not(res.buckets, a.buckets, COUNT(res.buckets));
    return res;
}

bool operator<(const u256 &a, const u256 &b) {
    for (s32 i = 0; i < 4; ++i) {
        if (a.buckets[i] < b.buckets[i]) return true;
        if (a.buckets[i] > b.buckets[i]) return false;
    }
    return false;
}

bool operator>(const u256 &a, const u256 &b) {
    for (s32 i = 0; i < 4; ++i) {
        if (a.buckets[i] > b.buckets[i]) return true;
        if (a.buckets[i] < b.buckets[i]) return false;
    }
    return false;
}

bool operator<(const u512 &a, const u512 &b) {
    for (s32 i = 0; i < 8; ++i) {
        if (a.buckets[i] < b.buckets[i]) return true;
        if (a.buckets[i] > b.buckets[i]) return false;
    }
    return false;
}

bool operator>(const u512 &a, const u512 &b) {
    for (s32 i = 0; i < 8; ++i) {
        if (a.buckets[i] > b.buckets[i]) return true;
        if (a.buckets[i] < b.buckets[i]) return false;
    }
    return false;
}
