#pragma once

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float  f32;
typedef double f64;

static_assert(sizeof(s8)  == 1);
static_assert(sizeof(u8)  == 1);
static_assert(sizeof(s16) == 2);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(s32) == 4);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(s64) == 8);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

#define S8_MIN  (-127i8 - 1)
#define S8_MAX    127i8
#define U8_MAX    0xffui8

#define S16_MIN (-32767i16 - 1)
#define S16_MAX   32767i16
#define U16_MAX   0xffffui16

#define S32_MIN (-2147483647i32 - 1)
#define S32_MAX   2147483647i32
#define U32_MAX   0xffffffffui32

#define S64_MIN (-9223372036854775807i64 - 1)
#define S64_MAX   9223372036854775807i64
#define U64_MAX   0xffffffffffffffffui64

#define F32_MIN     1.175494351e-38F
#define F32_MAX     3.402823466e+38F
#define F32_EPSILON 1.192092896e-07F

#define F64_MIN   2.2250738585072014e-308
#define F64_MAX   1.7976931348623158e+308

#define U32_PACK(a, b, c, d) ((u32)(a) << 0  | (u32)(b) << 8  | (u32)(c) << 16 | (u32)(d) << 24)

#define null nullptr

#define INVALID_INDEX -1
#define MAX_PATH_SIZE 256

#if DEBUG
inline const char* build_type_name = "DEBUG";
#elif RELEASE
inline const char* build_type_name = "RELEASE";
#else
#error "Unknown build type"
#endif

#define KB(n) (n * 1024ui64)
#define MB(n) (KB(n) * 1024ui64)
#define GB(n) (MB(n) * 1024ui64)
#define TB(n) (GB(n) * 1024ui64)

#define COUNT(a) sizeof(a) / sizeof((a)[0])

#define CONCAT_(a, b) a ## b 
#define CONCAT(a, b) CONCAT_(a, b)

struct Defer_Ref {};
template <class F> struct Defer { F f; ~Defer() { f(); } };
template <class F> Defer<F> operator+(Defer_Ref, F f) { return {f}; }
#define defer const auto CONCAT(deferrer, __LINE__) = Defer_Ref{} + [&]()

#define For(x) for (auto &it : (x))

#define RUN_TREE_FOLDER DIR_RUN_TREE
#define DATA_FOLDER     DIR_DATA
#define PACK_FOLDER     DIR_DATA
#define SHADER_FOLDER   DIR_SHADERS
#define TEXTURE_FOLDER  DIR_TEXTURES
#define SOUND_FOLDER    DIR_SOUNDS
#define FONT_FOLDER     DIR_FONTS

#define DATA_PATH(x)    DATA_FOLDER x
#define PACK_PATH(x)    PACK_FOLDER x
#define SHADER_PATH(x)  SHADER_FOLDER x
#define TEXTURE_PATH(x) TEXTURE_FOLDER x
#define SOUND_PATH(x)   SOUND_FOLDER x
#define FONT_PATH(x)    FONT_FOLDER x

#if DEBUG
#define Assert(x) if (x) {} else { report_assert(#x, __FILE__, __LINE__); }
void report_assert(const char* condition, const char* file, s32 line);
void debug_break();
#elif RELEASE
#define Assert(x)
#endif

enum Direction {
    DIRECTION_BACK,
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
    DIRECTION_FORWARD,
    DIRECTION_COUNT
};

inline constexpr u64 MAX_ALLOCL_SIZE = MB(65);
inline constexpr u64 MAX_ALLOCF_SIZE = MB(1);

bool alloc_init();
void alloc_shutdown();
void *alloch(u64 size);
void *realloch(void *ptr, u64 size);
void  freeh(void *ptr);
void *allocl(u64 size);
void  freel(u64 size);
void *allocf(u64 size);
void  freef();

#define allocht(T)     (T *)alloch(sizeof(T))
#define alloclt(T)     (T *)allocl(sizeof(T))
#define allocft(T)     (T *)allocf(sizeof(T))
#define allochtn(T, n) (T *)alloch(sizeof(T) * (n))
#define allocltn(T, n) (T *)allocl(sizeof(T) * (n))
#define allocftn(T, n) (T *)allocf(sizeof(T) * (n))

#define freelt(T)     freel(sizeof(T));
#define freeltn(T, n) freel(sizeof(T) * (n));
