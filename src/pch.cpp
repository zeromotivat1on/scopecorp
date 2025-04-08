#include "pch.h"
#include "sid.h"
#include "hash.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static u64 sid_table_hash(const u64 &a) {
    return a;
}

void init_sid_table(Sid_Table *sid_table) {
    *sid_table = Sid_Table(MAX_SID_TABLE_SIZE);
    sid_table->hash_function = &sid_table_hash;
}

u64 cache_sid(const char *string) {
    const u64 hash = hash_fnv(string);
    if (sid_table.find(hash) == null) sid_table.add(hash, string);
    return hash;
}

u32 hash_pcg32(u32 input) {
    const u32 state = input * 747796405u + 2891336453u;
    const u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u64 hash_pcg64(u64 input) {
    return hash_pcg32((u32)input) + hash_pcg32(input >> 32);
}

u64 hash_fnv(const char* str) {
    u64 hash = FNV_BASIS;
    for (const char* p = str; *p; ++p) {
        hash ^= (u64)(u8)(*p);
        hash *= FNV_PRIME;
    }

    return hash;
}
