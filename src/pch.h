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

#define U32_PACK(a, b, c, d) (u32)((a) << 0  | (b) << 8  | (c) << 16 | (d) << 24)

#define null nullptr

#if _MSC_VER
#define LITTLE_ENDIAN 1
#endif

typedef u64 sid; // string id
typedef u32 eid; // entity id

#if OPEN_GL
typedef u32 rid; // render id, used by underlying gfx api
#else
#error "Unsupported gfx api"
#endif

inline constexpr sid SID_NONE = 0;
inline constexpr eid EID_NONE = 0;
inline constexpr eid EID_MAX  = U32_MAX;
inline constexpr rid RID_NONE = 0;

#define rgba_pack(r, g, b, a) (u32)((r) << 24  | (g) << 16  | (b) << 8 | (a) << 0)

#define rgba_zero   rgba_pack(  0,   0,   0,   0)
#define rgba_black  rgba_pack(  0,   0,   0, 255)
#define rgba_white  rgba_pack(255, 255, 255, 255)
#define rgba_red    rgba_pack(255,   0,   0, 255)
#define rgba_green  rgba_pack(  0, 255,   0, 255)
#define rgba_blue   rgba_pack(  0,   0, 255, 255)
#define rgba_yellow rgba_pack(255, 255,   0, 255)
#define rgba_purple rgba_pack(255,   0, 255, 255)
#define rgba_cyan   rgba_pack(  0, 255, 255, 255)

#define ASCII_BACKSPACE       8
#define ASCII_TAB             9
#define ASCII_NEW_LINE        10
#define ASCII_VERTICAL_TAB    11
#define ASCII_FORM_FEED       12
#define ASCII_CARRIAGE_RETURN 13
#define ASCII_ESCAPE          27
#define ASCII_SPACE           32
#define ASCII_GRAVE_ACCENT    96

#define is_ascii(x)           ((x) >= 0 && (x) <= 127)
#define is_ascii_ext(x)       ((x) >= 0 && (x) <= 255)
#define is_ascii_printable(x) ((x) >= 32 && (x) <= 126)

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

struct My_Defer_Ref {};
template <class F> struct My_Defer { F f; ~My_Defer() { f(); } };
template <class F> My_Defer<F> operator+(My_Defer_Ref, F f) { return {f}; }
#define defer const auto CONCAT(deferrer, __LINE__) = My_Defer_Ref{} + [&]()

#define For(x) for (auto &it : (x))

#define Align(x, alignment) (((x) + (alignment) - 1) & ~((alignment) - 1))

#define Sign(x)        ((x) > 0 ? 1 : -1)
#define Min(a, b)      ((a) < (b) ? (a) : (b))
#define Max(a, b)      ((a) > (b) ? (a) : (b))
#define Clamp(x, a, b) (Min((b), Max((a), (x))))

#define PATH_DATA(x)      DIR_DATA x
#define PATH_PACK(x)      DIR_DATA x
#define PATH_SHADER(x)    DIR_SHADERS x
#define PATH_TEXTURE(x)   DIR_TEXTURES x
#define PATH_MATERIAL(x)  DIR_MATERIALS x
#define PATH_MESH(x)      DIR_MESHES x
#define PATH_SOUND(x)     DIR_SOUNDS x
#define PATH_FONT(x)      DIR_FONTS x
#define PATH_FLIP_BOOK(x) DIR_FLIP_BOOKS x
#define PATH_LEVEL(x)     DIR_LEVELS x

#if DEVELOPER
#define Assert(x) if (x) {} else { report_assert(#x, __FILE__, __LINE__); }
void report_assert(const char* condition, const char* file, s32 line);
void debug_break();
#else
#define Assert(x)
#endif

// Hint compiler to not reorder memory operations, happened before barrier.
// Basically disable memory read/write concerned optimizations.
void read_barrier();
void write_barrier();
void memory_barrier();

// Ensure the order of read/write CPU instructions.
void read_fence();
void write_fence();
void memory_fence();

enum Direction {
    DIRECTION_BACK,
    DIRECTION_RIGHT,
    DIRECTION_LEFT,
    DIRECTION_FORWARD,
    DIRECTION_COUNT
};

inline constexpr u64 MAX_ALLOCL_SIZE = MB(64);
inline constexpr u64 MAX_ALLOCF_SIZE = MB(1);

// s  - stack allocation, default implementation
// h  - heap allocation, default implementation
// l  - linear allocation, push/pop bytes
// f  - frame allocation, cleared at the end of every frame
// lp - linear allocation from given pointer, push/pop bytes

bool alloc_init();
void alloc_shutdown();
void *allocs(u64 size);
void *alloch(u64 size);
void *realloch(void *ptr, u64 size);
void  freeh(void *ptr);
void *allocl(u64 size);
void  freel(u64 size);
void *allocf(u64 size);
void  freef();
void *alloclp(void **ptr, u64 size);
void  freelp(void **ptr, u64 size);

#define allocht(T)     (T *)alloch(sizeof(T))
#define alloclt(T)     (T *)allocl(sizeof(T))
#define allocft(T)     (T *)allocf(sizeof(T))
#define allochtn(T, n) (T *)alloch(sizeof(T) * (n))
#define allocltn(T, n) (T *)allocl(sizeof(T) * (n))
#define allocftn(T, n) (T *)allocf(sizeof(T) * (n))

#define freelt(T)     freel(sizeof(T));
#define freeltn(T, n) freel(sizeof(T) * (n));

void set_bytes (void *data, s32 value, u64 size);
void copy_bytes(void *dst, const void *src, u64 size);
void move_bytes(void *dst, const void *src, u64 size);
