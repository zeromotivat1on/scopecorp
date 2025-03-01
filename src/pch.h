#pragma once

#include <assert.h>

typedef signed char        s8;
typedef short              s16;
typedef int                s32;
typedef long long          s64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float  f32;
typedef double f64;

static_assert(sizeof(s8) == 1);
static_assert(sizeof(u8) == 1);
static_assert(sizeof(s16) == 2);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(s32) == 4);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(s64) == 8);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

#define MIN_S8  (-127i8 - 1)
#define MAX_S8    127i8
#define MAX_U8    0xffui8

#define MIN_S16 (-32767i16 - 1)
#define MAX_S16   32767i16
#define MAX_U16   0xffffui16

#define MIN_S32 (-2147483647i32 - 1)
#define MAX_S32   2147483647i32
#define MAX_U32   0xffffffffui32

#define MIN_S64 (-9223372036854775807i64 - 1)
#define MAX_S64   9223372036854775807i64
#define MAX_U64   0xffffffffffffffffui64

#define MIN_F32   1.175494351e-38F
#define MAX_F32   3.402823466e+38F

#define MIN_F64   2.2250738585072014e-308
#define MAX_F64   1.7976931348623158e+308

#define null nullptr

#define INVALID_INDEX -1
#define MAX_PATH_SIZE 256

#if DEBUG

inline const char* build_type_name = "DEBUG";
#define debug_scope if (1)

#elif RELEASE

inline const char* build_type_name = "RELEASE";
#define debug_scope if (0)

#else
#error "Unknown build type"
#endif

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

enum Direction {
    DIRECTION_BACK,
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
    DIRECTION_FORWARD,
    DIRECTION_COUNT
};
