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
#define U8_MAX    0xFFui8

#define S16_MIN (-32767i16 - 1)
#define S16_MAX   32767i16
#define U16_MAX   0xFFFFui16

#define S32_MIN (-2147483647i32 - 1)
#define S32_MAX   2147483647i32
#define U32_MAX   0xFFFFFFFui32

#define S64_MIN (-9223372036854775807i64 - 1)
#define S64_MAX   9223372036854775807i64
#define U64_MAX   0xFFFFFFFFFFFFFui64

#define F32_MIN     1.175494351e-38F
#define F32_MAX     3.402823466e+38F
#define F32_EPSILON 1.192092896e-07F

#define F64_MIN   2.2250738585072014E-308
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
#error "Unsupported graphics api"
#endif

inline constexpr sid SID_NONE = 0;
inline constexpr eid EID_NONE = 0;
inline constexpr eid EID_MAX  = U32_MAX;
inline constexpr rid RID_NONE = 0;

#define INDEX_NONE -1
#define MAX_PATH_LENGTH 256

#define rgba_pack(r, g, b, a) (u32)((r) << 24  | (g) << 16  | (b) << 8 | (a) << 0)

#define rgba_set_r(c, r) (((c) & 0x00FFFFFF) | ((u32)(r) << 24))
#define rgba_set_g(c, g) (((c) & 0xFF00FFFF) | ((u32)(g) << 16))
#define rgba_set_b(c, b) (((c) & 0xFFFF00FF) | ((u32)(b) << 8))
#define rgba_set_a(c, a) (((c) & 0xFFFFFF00) | ((u32)(a) << 0))

#define rgba_get_r(c) (((c) >> 24) & 0xFF)
#define rgba_get_g(c) (((c) >> 16) & 0xFF)
#define rgba_get_b(c) (((c) >> 8)  & 0xFF)
#define rgba_get_a(c) (((c) >> 0)  & 0xFF)

#define rgba_white  rgba_pack(255, 255, 255, 255)
#define rgba_black  rgba_pack(  0,   0,   0, 255)
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
#define is_ascii_ex(x)        ((x) >= 0 && (x) <= 255)
#define is_ascii_printable(x) ((x) >= 32 && (x) <= 126)

#if DEBUG
inline const char* Build_type_name = "DEBUG";
#elif RELEASE
inline const char* Build_type_name = "RELEASE";
#else
#error "Unknown build type"
#endif

#define KB(n) (n * 1024)
#define MB(n) (n * 1024 * 1024)
#define GB(n) (n * 1024 * 1024 * 1024)
#define TB(n) (n * 1024 * 1024 * 1024 * 1024)

#define TO_KB(n) (n / 1024)
#define TO_MB(n) (n / 1024 / 1024)
#define TO_GB(n) (n / 1024 / 1024 / 1024)
#define TO_TB(n) (n / 1024 / 1024 / 1024 / 1024)

#define COUNT(a) sizeof(a) / sizeof((a)[0])

#define CONCAT_(a, b) a ## b 
#define CONCAT(a, b) CONCAT_(a, b)

struct My_Defer_Ref {};
template <class F> struct My_Defer { F f; ~My_Defer() { f(); } };
template <class F> My_Defer<F> operator+(My_Defer_Ref, F f) { return {f}; }
#define defer const auto CONCAT(deferrer, __LINE__) = My_Defer_Ref{} + [&]()

#define For(x) for (auto &it : (x))

#define Align(x, alignment) (((x) + (alignment) - 1) & ~((alignment) - 1))

#define Offsetof(x, m) ((u64)&(((x *)0)->m))

#define Sign(x)        ((x) > 0 ? 1 : -1)
#define Min(a, b)      ((a) < (b) ? (a) : (b))
#define Max(a, b)      ((a) > (b) ? (a) : (b))
#define Clamp(x, a, b) (Min((b), Max((a), (x))))
#define Lerp(a, b, t)  ((a) + ((b) - (a)) * (t))

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

struct Source_Location {
    const char *file = null;
    s32 line = 0;

    Source_Location(const char *file, u32 line)
        : file(file), line(line) {}

    static Source_Location current(const char *file = __builtin_FILE(), s32 line = __builtin_LINE()) {
        return Source_Location(file, line);
    }
};

#if DEVELOPER
#define Assert(x)       if (x) {} else { report_assert(#x, null); }
#define Assertm(x, msg) if (x) {} else { report_assert(#x, msg);  }
void report_assert(const char* condition, const char *msg, Source_Location loc = Source_Location::current());
void debug_break();
#else
#define Assert(x)
#define Assertm(x, msg)
#endif

// Hint compiler to not reorder memory operations, happened before barrier.
// Basically disable memory read/write concerned optimizations.
void asm_read_barrier();
void asm_write_barrier();
void asm_memory_barrier();

// Ensure the order of read/write CPU instructions.
void cpu_read_fence();
void cpu_write_fence();
void cpu_memory_fence();

enum For_Result : u8 {
    CONTINUE,
    BREAK,
};

enum Direction : u8 {
    SOUTH,
    EAST,
    WEST,
    NORTH,
    DIRECTION_COUNT
};

// Allocation types:
// s  - stack allocation, default implementation
// h  - heap allocation, default implementation
// p  - persistent storage, push/pop
// t  - temporary storage, push/pop
// f  - frame storage, cleared at the end of every frame, push/pop

inline constexpr u64 MAX_ALLOCP_SIZE = MB(16);
inline constexpr u64 MAX_ALLOCT_SIZE = MB(64);
inline constexpr u64 MAX_ALLOCF_SIZE = MB(1);

bool alloc_init();
void alloc_shutdown();
void *allocs(u64 size);
void *alloch(u64 size);
void *realloch(void *ptr, u64 size);
void  freeh(void *ptr);
void *allocp(u64 size, Source_Location loc = Source_Location::current());
void  freep(u64 size, Source_Location loc = Source_Location::current());
void *alloct(u64 size);
void  freet(u64 size);
void *allocf(u64 size);
void  freef(u64 size);
void  freef();

#define allocsn(T, n) (T *)allocs(sizeof(T) * (n))
#define allochn(T, n) (T *)alloch(sizeof(T) * (n))
#define allocpn(T, n) (T *)allocp(sizeof(T) * (n))
#define alloctn(T, n) (T *)alloct(sizeof(T) * (n))
#define allocfn(T, n) (T *)allocf(sizeof(T) * (n))

#define freepn(T, n) freep(sizeof(T) * (n));
#define freetn(T, n) freet(sizeof(T) * (n));
#define freefn(T, n) freef(sizeof(T) * (n));

void mem_set (void *data, s32 value, u64 size);
void mem_copy(void *dst, const void *src, u64 size);
void mem_move(void *dst, const void *src, u64 size);
void mem_swap(void *a, void *b, u64 size);

void sort(void *data, u32 count, u32 size, s32 (*compare)(const void *, const void *));

// All R_ defines must NOT exceed u16 range.

#define R_NONE 0

#define R_ENABLE  1
#define R_DISABLE 2

#define R_TRIANGLES      10
#define R_TRIANGLE_STRIP 11
#define R_LINES          12

#define R_FILL  20
#define R_LINE  21
#define R_POINT 22

#define R_CW  30
#define R_CCW 31

#define R_BACK        40
#define R_FRONT       41
#define R_FRONT_BACK  42

#define R_SRC_ALPHA           50
#define R_ONE_MINUS_SRC_ALPHA 51

#define R_LESS    60
#define R_GREATER 61
#define R_EQUAL   62
#define R_NEQUAL  63
#define R_LEQUAL  64
#define R_GEQUAL  65

#define R_ALWAYS  66
#define R_KEEP    67
#define R_REPLACE 68

#define R_S32     70
#define R_U32     71
#define R_F32     72
#define R_F32_2   73
#define R_F32_3   74
#define R_F32_4   75
#define R_F32_4X4 76

#define R_TEXTURE_2D       100
#define R_TEXTURE_2D_ARRAY 101

#define R_RED_32             110
#define R_RGB_8              111
#define R_RGBA_8             112
#define R_DEPTH_24_STENCIL_8 113

#define R_REPEAT 120
#define R_CLAMP  121
#define R_BORDER 123

#define R_LINEAR                 130
#define R_NEAREST                131
#define R_LINEAR_MIPMAP_LINEAR   133
#define R_LINEAR_MIPMAP_NEAREST  134
#define R_NEAREST_MIPMAP_LINEAR  135
#define R_NEAREST_MIPMAP_NEAREST 136

#define R_COLOR_BUFFER_BIT   0x1
#define R_DEPTH_BUFFER_BIT   0x2
#define R_STENCIL_BUFFER_BIT 0x4

#define R_DYNAMIC_STORAGE_BIT 0x1
#define R_CLIENT_STORAGE_BIT  0x2
// Custom bit. Tells whether storage is already mapped.
#define R_MAPPED_STORAGE_BIT  0x4
#define R_MAP_READ_BIT        0x8
#define R_MAP_WRITE_BIT       0x10
#define R_MAP_PERSISTENT_BIT  0x20
#define R_MAP_COHERENT_BIT    0x40
