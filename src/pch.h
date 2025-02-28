#pragma once

#include <stdint.h>
#include <assert.h>

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

#define null nullptr

#define INVALID_INDEX -1

#define KB(n) (n * 1024ULL)
#define MB(n) (KB(n) * 1024ULL)
#define GB(n) (MB(n) * 1024ULL)
#define TB(n) (GB(n) * 1024ULL)

#define c_array_count(a) sizeof(a) / sizeof((a)[0])

#define _CONCAT(a, b) a ## b 
#define  CONCAT(a, b) _CONCAT(a, b)

struct Defer_Ref {};
template <class F> struct Defer { F f; ~Defer() { f(); } };
template <class F> Defer<F> operator+(Defer_Ref, F f) { return {f}; }
#define defer const auto CONCAT(deferrer, __LINE__) = Defer_Ref{} + [&]()

#if DEBUG
inline const char* build_type_name = "DEBUG";
#elif RELEASE
inline const char* build_type_name = "RELEASE";
#else
#error "Unknown build type"
#endif

enum Direction {
    BACK,
    RIGHT,
    LEFT,
    FORWARD,
    DIRECTION_COUNT
};

inline constexpr s32 MAX_PATH_SIZE = 256;
