#pragma once

inline u16 swap_endianness_16(const void* data) {
    const u8 *mem = (u8 *)data;
    return ((u16)mem[0] << 8) | ((u16)mem[1] << 0);
}

inline u32 swap_endianness_32(const void* data) {
    const u8 *mem = (u8 *)data;
    return ((u32)mem[0] << 24) | ((u32)mem[1] << 16) | ((u32)mem[2] << 8) | ((u32)mem[3] << 0);
}

inline u64 swap_endianness_64(const void* data) {
    const u8 *mem = (u8 *)data;
    return ((u64)mem[0] << 56) | ((u64)mem[1] << 48) | ((u64)mem[2] << 40) | ((u64)mem[3] << 32) |
           ((u64)mem[4] << 24) | ((u64)mem[5] << 16) | ((u64)mem[6] <<  8) | ((u64)mem[7] <<  0);
}

inline void* eat(void** data, u32 bytes) {
    void* ptr = *data;
    *data = (u8 *)(*data) + bytes;
    return ptr;
}

inline char eat_char(void** data) {
    return *(char *)eat(data, sizeof(char));
}

inline u8 eat_u8(void** data) {
    return *(u8 *)eat(data, sizeof(u8));
}

inline s16 eat_s16(void** data) {
    return *(s16 *)eat(data, sizeof(s16));
}

inline u16 eat_u16(void** data) {
    return *(u16 *)eat(data, sizeof(u16));
}

inline s32 eat_s32(void** data) {
    return *(s32 *)eat(data, sizeof(s32));
}

inline u32 eat_u32(void** data) {
    return *(u32 *)eat(data, sizeof(u32));
}

inline s64 eat_s64(void** data) {
    return *(s64 *)eat(data, sizeof(s64));
}

inline u64 eat_u64(void** data) {
    return *(u64 *)eat(data, sizeof(u64));
}

inline f32 eat_f2dot14(void** data) {
    const s16 val = eat_s16(data);
    return val / (f32)(1 << 14);
}

inline s16 eat_big_endian_s16(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_16(eat(data, sizeof(s16)));
#else
    return eat_s16(data);
#endif
}

inline u16 eat_big_endian_u16(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_16(eat(data, sizeof(u16)));
#else
    return eat_u16(data);
#endif
}

inline s32 eat_big_endian_s32(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_32(eat(data, sizeof(s32)));
#else
    return eat_s32(data);
#endif
}

inline u32 eat_big_endian_u32(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_32(eat(data, sizeof(u32)));
#else
    return eat_u32(data);
#endif
}

inline s64 eat_big_endian_s64(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_64(eat(data, sizeof(s64)));
#else
    return eat_s64(data);
#endif
}

inline u64 eat_big_endian_u64(void** data) {
#if LITTLE_ENDIAN
    return swap_endianness_64(eat(data, sizeof(u64)));
#else
    return eat_u64(data);
#endif
}

inline f32 eat_big_endian_f2dot14(void** data) {
    s16 val = eat_big_endian_s16(data);    
    return val / (f32)(1 << 14);
}
