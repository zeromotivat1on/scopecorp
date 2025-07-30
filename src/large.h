#pragma once

struct u128 {
    u64 buckets[2] = { 0 };
};

struct u256 {
    u64 buckets[4] = { 0 };
};

struct u512 {
    u64 buckets[8] = { 0 };
};

void set(u64 *buckets, u16 pos);
void clear(u64 *buckets, u16 pos);
void toggle(u64 *buckets, u16 pos);
bool check(const u64 *buckets, u16 pos);
void set_whole(u64 *buckets, u8 count, u8 start = 0);
void clear_whole(u64 *buckets, u8 count, u8 start = 0);
void And(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count);
void Or(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count);
void Xor(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count);
void Not(u64 *res_buckets, const u64 *buckets, u8 count);

u256 operator&(const u256 &a, const u256 &b);
u256 operator|(const u256 &a, const u256 &b);
u256 operator^(const u256 &a, const u256 &b);
u256 operator~(const u256 &a);
bool operator<(const u256 &a, const u256 &b);
bool operator>(const u256 &a, const u256 &b);

bool operator<(const u512 &a, const u512 &b);
bool operator>(const u512 &a, const u512 &b);
