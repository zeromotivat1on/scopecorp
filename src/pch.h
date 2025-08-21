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
#define is_ascii_space(x)     ((x) == ASCII_SPACE               \
                               || (x) == ASCII_TAB              \
                               || (x) == ASCII_NEW_LINE         \
                               || (x) == ASCII_FORM_FEED        \
                               || (x) == ASCII_VERTICAL_TAB     \
                               || (x) == ASCII_CARRIAGE_RETURN)

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

#define _sizeref(x) sizeof(x), &x

struct My_Defer_Ref {};
template <class F> struct My_Defer { F f; ~My_Defer() { f(); } };
template <class F> My_Defer<F> operator+(My_Defer_Ref, F f) { return {f}; }
#define defer const auto CONCAT(_defer_, __LINE__) = My_Defer_Ref{} + [&]()

#define For(x) for (auto &it : (x))

#define Alignup(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define Aligndown(x, a) ((x) & ~((a) - 1))
#define Offsetof(x, m)  ((u64)&(((x *)0)->m))
#define Powerof2(x)     (((x) != 0) && ((x) & ((x) - 1)) == 0)

#define Sign(x)        ((x) > 0 ? 1 : -1)
#define Min(a, b)      ((a) < (b) ? (a) : (b))
#define Max(a, b)      ((a) > (b) ? (a) : (b))
#define Clamp(x, a, b) (Min((b), Max((a), (x))))
#define Lerp(a, b, t)  ((a) + ((b) - (a)) * (t))

#define PATH_DATA(x)      S(DIR_DATA x)
#define PATH_PACK(x)      S(DIR_DATA x)
#define PATH_SHADER(x)    S(DIR_SHADERS x)
#define PATH_TEXTURE(x)   S(DIR_TEXTURES x)
#define PATH_MATERIAL(x)  S(DIR_MATERIALS x)
#define PATH_MESH(x)      S(DIR_MESHES x)
#define PATH_SOUND(x)     S(DIR_SOUNDS x)
#define PATH_FONT(x)      S(DIR_FONTS x)
#define PATH_FLIP_BOOK(x) S(DIR_FLIP_BOOKS x)
#define PATH_LEVEL(x)     S(DIR_LEVELS x)

struct Source_Location {
    const char *file = null;
    s32 line = 0;

    Source_Location(const char *file, u32 line) : file(file), line(line) {}

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

struct Rect {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 w = 0.0f;
    f32 h = 0.0f;
};

struct Buffer {
    u8 *data = null;
    u64 size = 0;
};

#define BUFFER_NONE Buffer { null, 0 }

bool is_valid(Buffer b);

// Self-contained memory storage.
struct Arena {
    u8 *base = null;
    u64 reserved = 0;
    u64 commited = 0;
    u64 used = 0;
};

// Temp view of memory storage.
struct Scratch {
    Arena &arena;
    u64 pos = 0;
};

inline Arena M_global; // application lifetime allocations
inline Arena M_frame;  // frame lifetime allocations

inline constexpr u64 DEFAULT_ALIGNMENT = 8;

bool  reserve  (Arena &a, u64 n, void *address = null);
bool  commit   (Arena &a, u64 n);
void *push     (Arena &a, u64 n, u64 align = DEFAULT_ALIGNMENT, bool zero = true);
void  pop      (Arena &a, u64 n);
void  clear    (Arena &a);
bool  decommit (Arena &a, u64 n);
bool  release  (Arena &a);

// Returns thread local scratch for memory allocation.
// There are several scratch storages under the hood for each thread, so
// it is safe to have not strict push/pop calls in case of nested scratches.
Scratch local_scratch();
void release(Scratch &s);

#define arena_push_type(a, t)     (t *)push(a, (1) * sizeof(t), alignof(t), true)
#define arena_push_array(a, n, t) (t *)push(a, (n) * sizeof(t), alignof(t), true)
#define arena_push_buffer(a, n)   Buffer { (u8 *)push(a, n), n }
#define arena_push_string(a, n)   String { (char *)push(a, n), n }
#define arena_pop_type(a, t)      pop(a, (1) * sizeof(t))
#define arena_pop_array(a, n, t)  pop(a, (n) * sizeof(t))

void mem_set  (void *p, s32 v, u64 n);
void mem_copy (void *dst, const void *src, u64 n);
void mem_move (void *dst, const void *src, u64 n);
void mem_swap (void *a, void *b, u64 n);

// Simple wrapper mainly for string literals as most of functionality over it does not
// deal with internal memory, so standart copy is basically copy of pointer and length.
// Functions that actually do something with underlying memory take Arena to alloc from.
// It's up to the client to use it properly and do not shoot himself in the foot with
// basic error like changing contents of string literal.
// It's encouraged to use this String over raw c-strings or any other string wrappers.
struct String {
    char *value = null;
    u64 length  = 0;
};

// Iterator used for string tokenization.
struct String_Token_Iterator {
    String &s;
    String delims;
    u64 pos = 0;
};

#define STRING_NONE String { null, 0 }
#define S(literal)  String { literal, sizeof(literal) - 1 }

#define STI(s, delims) String_Token_Iterator { s, delims, 0 }

#define S_SEARCH_REVERSE_BIT  0x1
#define S_INDEX_PLUS_ONE_BIT  0x2
#define S_INDEX_MINUS_ONE_BIT 0x4
#define S_LENGTH_ON_FAIL_BIT  0x8
#define S_START_PLUS_ONE_BIT  0x10
#define S_LEFT_SLICE_BIT      0x20

// @Note: this ones were added just to let C++ know how to compare POD type...
inline bool operator==(const String &a, const String &b) { return a.value == b.value && a.length == b.length; }
inline bool operator!=(const String &a, const String &b) { return !(a == b); }

String s           (const char *cs);
String str_copy    (Arena &a, const char *cs);
String str_copy    (Arena &a, const char *cs, u64 length);
String str_copy    (Arena &a, String s);
String str_format  (Arena &a, const char *cs, ...);
String str_from    (String s, s64 i);
String str_from_to (String s, s64 a, s64 b);
String str_trim    (String s);
String str_slice   (String s, char c, u32 bits = 0);
String str_slice   (String s, String sub, u32 bits = 0);
String str_token   (String_Token_Iterator &it);
void   str_c       (String s, u64 length, char *cs);
bool   str_equal   (String a, String b);
s32    str_compare (String a, String b);
s64    str_index   (String s, char c, u32 bits = 0);
s64    str_index   (String s, String sub, u32 bits = 0);
s8     str_to_s8   (String s);
s16    str_to_s16  (String s);
s32    str_to_s32  (String s);
s64    str_to_s64  (String s);
u8     str_to_u8   (String s);
u16    str_to_u16  (String s);
u32    str_to_u32  (String s);
u64    str_to_u64  (String s);
f32    str_to_f32  (String s);

bool is_valid(String s);

struct String_Builder {
    String buffer;
    u64 capacity = 0;
};

void   str_build        (Arena &a, String_Builder &sb, const char *cs);
void   str_build        (Arena &a, String_Builder &sb, const char *cs, u64 length);
void   str_build        (Arena &a, String_Builder &sb, String s);
void   str_build_format (Arena &a, String_Builder &sb, const char *cs, ...);
String str_build_finish (Arena &a, String_Builder &sb);

// @Speed: this will intern sid on every call which may be not so optimal.
#define SID(s) sid_intern(s)

void sid_init();
sid  sid_intern(const char *cs);
sid  sid_intern(String s);
String sid_str(sid sid);

#if DEVELOPER
#define TM_PUSH_ZONE(s)  telemetry_push_zone(S(s))
#define TM_POP_ZONE()    telemetry_pop_zone()
#define TM_SCOPE_ZONE(s) TM_PUSH_ZONE(s); defer { TM_POP_ZONE(); }
#else
#define TM_PUSH_ZONE(s)
#define TM_POP_ZONE()
#define TM_SCOPE_ZONE(s)
#endif

void sort(void *data, u32 count, u32 size, s32 (*compare)(const void *, const void *));

// All R_ defines does NOT exceed u16 range, except for bit-flags of course.

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
