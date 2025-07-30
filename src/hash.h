#pragma once

inline constexpr u64 FNV_BASIS = 14695981039346656037;
inline constexpr u64 FNV_PRIME = 1099511628211;

u32 hash_pcg32(u32 input);
u64 hash_pcg64(u64 input);
u64 hash_fnv(String s);
