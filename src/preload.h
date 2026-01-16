#pragma once

#include <new>
#include <stdarg.h>

#ifdef _WIN32
#define WIN32 1
#else
#error "Unsupported platform"
#endif

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef float              f32;
typedef double             f64;
#define null               nullptr

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

#define S8_MIN    (-127i8 - 1)
#define S8_MAX      127i8
#define U8_MAX      0xFFui8
#define S16_MIN   (-32767i16 - 1)
#define S16_MAX     32767i16
#define U16_MAX     0xFFFFui16
#define S32_MIN   (-2147483647i32 - 1)
#define S32_MAX     2147483647i32
#define U32_MAX     0xFFFFFFFFui32
#define S64_MIN   (-9223372036854775807i64 - 1)
#define S64_MAX     9223372036854775807i64
#define U64_MAX     0xFFFFFFFFFFFFFFFFui64
#define F32_MIN     1.175494351e-38F
#define F32_MAX     3.402823466e+38F
#define F32_EPSILON 1.192092896e-07F
#define F64_MIN     2.2250738585072014E-308
#define F64_MAX     1.7976931348623158e+308

#define U16_PACK(a, b)                   (u16)((a) << 0  | (b) << 8)
#define U32_PACK(a, b, c, d)             (u32)((a) << 0  | (b) << 8  | (c) << 16 | (d) << 24)
#define U64_PACK(a, b, c, d, e, f, g, h) (u64)((a) << 0  | (b) << 8  | (c) << 16 | (d) << 24 | (e) << 32 | (f) << 40 | (g) << 48 | (h) << 56)

#define INDEX_NONE -1

typedef u32 Pid; // persistent identifier

// Opaque type that represents a reference to system's underlying resource.
union Handle {
    u16   _u16;
    u32   _u32;
    u64   _u64;
    void *_ptr;

    explicit operator bool() const { return _ptr; }
};

inline bool operator==(Handle a, Handle b) { return a._u64 == b._u64; }
inline bool operator!=(Handle a, Handle b) { return !(a == b); }
inline bool is_valid  (Handle a)           { return a._ptr; }

struct String {
    u8 *data = null;
    u64 size = 0;

    explicit operator bool() const { return data && size; }
};

inline bool is_valid(String s) { return s.data && s.size; }

struct Buffer {
    u8 *data = null;
    u64 size = 0;
    
    explicit operator bool() const { return data && size; }
};

inline bool   is_valid    (Buffer b)             { return b.data && b.size; }
inline Buffer make_buffer (void *data, u64 size) { return { .data = (u8 *)data, .size = size }; }
inline Buffer make_buffer (String s)             { return { .data = (u8 *)s.data, .size = s.size }; }

union u128 {
    u8  _u8 [16];
    u16 _u16[8];
    u32 _u32[4];
    u64 _u64[2];
};

union u256 {
    u8   _u8  [32];
    u16  _u16 [16];
    u32  _u32 [8];
    u64  _u64 [4];
    u128 _u128[2];
};

union u512 {
    u8   _u8  [64];
    u16  _u16 [32];
    u32  _u32 [16];
    u64  _u64 [8];
    u128 _u128[4];
    u256 _u256[2];
};

void set_bit    (void *p, u64 n);
void clear_bit  (void *p, u64 n);
void toggle_bit (void *p, u64 n);
bool check_bit  (const void *p, u64 n);

void __and (void *r, const void *a, const void *b, u64 n);
void __or  (void *r, const void *a, const void *b, u64 n);
void __xor (void *r, const void *a, const void *b, u64 n);
void __not (void *r, const void *a, u64 n);

u128 operator& (const u128 &a, const u128 &b);
u256 operator& (const u256 &a, const u256 &b);
u512 operator& (const u512 &a, const u512 &b);
u128 operator| (const u128 &a, const u128 &b);
u256 operator| (const u256 &a, const u256 &b);
u512 operator| (const u512 &a, const u512 &b);
u128 operator^ (const u128 &a, const u128 &b);
u256 operator^ (const u256 &a, const u256 &b);
u512 operator^ (const u512 &a, const u512 &b);
u128 operator~ (const u128 &a);
u256 operator~ (const u256 &a);
u512 operator~ (const u512 &a);

inline constexpr auto PI = 3.14159265358979323846f;

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT 8
#endif

#ifndef DEFAULT_PAGE_SIZE
extern u64 get_page_size ();
inline const auto __page_size = get_page_size();
#define DEFAULT_PAGE_SIZE __page_size
#endif

extern u64 get_perf_counter ();
inline const auto __preload_counter = get_perf_counter();

#define unreachable_code_path() Assert(0, "Unreachable code path")

#define __expand(x)     x
#define __concat2(a, b) a ## b
#define __concat(a, b)  __concat2(a, b)

#define carray_count(x) (sizeof(x) / sizeof((x)[0]))

#define offset_of(x, m) ((u64)&(((x *)0)->m))

#define Align(x, a)        (((x) + (a) - 1) & ~((a) - 1))
#define Align_Down(x, a)   ((x) & ~((a) - 1))
#define Is_Power_Of_Two(x) (((x) != 0) && ((x) & ((x) - 1)) == 0)

#define Kilobytes(n) (n * 1024)
#define Megabytes(n) (n * 1024 * 1024)
#define Gigabytes(n) (n * 1024 * 1024 * 1024)
#define Terabytes(n) (n * 1024 * 1024 * 1024 * 1024)

#define To_Kilobytes(n) (n / 1024)
#define To_Megabytes(n) (n / 1024 / 1024)
#define To_Gigabytes(n) (n / 1024 / 1024 / 1024)
#define To_Terabytes(n) (n / 1024 / 1024 / 1024 / 1024)

#define To_Radians(d) (d * (PI / 180.0f))
#define To_Degrees(r) (r * (180.0f / PI))

#define Abs(x)         ((x) < 0 ? -(x) : (x))
#define Sign(x)        ((x) > 0 ? 1 : -1)
#define Sqr(x)         ((x) * (x))
#define Min(a, b)      (((a) < (b)) ? (a) : (b))
#define Max(a, b)      (((a) > (b)) ? (a) : (b))
#define Clamp(x, a, b) (Min((b), Max((a), (x))))
#define Lerp(a, b, t)  ((a) + ((b) - (a)) * (t))

#define Is_Ascii(c)     ((c) >=  0 && (c) <= 127)
#define Is_Ascii_Ex(c)  ((c) >=  0 && (c) <= 255)
#define Is_Printable(c) ((c) >= 32 && (c) <= 126)
#define Is_Space(c)     ((c) == 32 || ((c) >=  9 && (c) <=  13))

void *eat (void **data, u64 n);
#define __get_eat_macro(_1, _2, _3, name, ...) name
#define __eat1(x, T)    (T *)eat((x), (1) * sizeof(T))
#define __eat2(x, T, n) (T *)eat((x), (n) * sizeof(T))
#define Eat(...) __expand(__get_eat_macro(__VA_ARGS__, __eat2, __eat1)(__VA_ARGS__))

struct __Defer2 {};
template <class F> struct __Defer { F f; ~__Defer() { f(); } };
template <class F> __Defer<F> operator+(__Defer2, F f) { return {f}; }
#define defer const auto __concat(__defer_, __LINE__) = __Defer2{} + [&]()

#define For(x) for (auto &it : (x))

struct Source_Code_Location {
    const char *file     = null;
    const char *function = null;
    s32 line = 0;
};

#define __location Source_Code_Location { .file = __builtin_FILE(), .function = __builtin_FUNCTION(), .line = __builtin_LINE() }

enum Allocator_Mode : u8 {
    ALLOCATE,
    RESIZE,
    FREE,
    FREE_ALL,
};

typedef void *(*Allocator_Proc) (Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data);
void *__default_allocator_proc  (Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data);

struct Allocator {
    Allocator_Proc proc = null;
    void          *data = null;
};

inline thread_local Allocator __default_allocator = { .proc = __default_allocator_proc, .data = null };

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL LOG_VERBOSE
#endif

enum Log_Level : u8 {
    LOG_VERBOSE,
    LOG_DEFAULT,
    LOG_WARNING,
    LOG_ERROR,
};

typedef void  (*Logger_Proc) (String message, String ident, Log_Level level, void *logger_data);
void  __default_logger_proc  (String message, String ident, Log_Level level, void *logger_data);

struct Logger {
    Logger_Proc proc = null;
    void       *data = null;
};

inline thread_local Logger __default_logger = { .proc = __default_logger_proc, .data = null };

typedef void (*Assert_Proc) (const char *condition, Source_Code_Location loc, const char *format, ...);
void  __default_assert_proc (const char *condition, Source_Code_Location loc, const char *format, ...);

#define __get_assert_macro(_1, _2, _3, name, ...) name
#define __assert_body(x, ...) if (x) {} else { context.assert_proc(#x, __location, __VA_ARGS__); }
#define __assert1(x)         __assert_body(x, null)
#define __assert2(x, m)      __assert_body(x, m)
#define __assert3(x, m, ...) __assert_body(x, m, __VA_ARGS__)
#define Assert(...) __expand(__get_assert_macro(__VA_ARGS__, __assert3, __assert2, __assert1)(__VA_ARGS__))

void read_barrier   ();
void write_barrier  ();
void memory_barrier ();

void read_fence   ();
void write_fence  ();
void memory_fence ();

void set       (void *p, s32 v, u64 n);
void copy      (void *dst, const void *src, u64 n);
void safe_copy (void *dst, const void *src, u64 n);

char *cstring_copy    (const char *s);
char *cstring_copy    (const char *s, Allocator alc);
u64   cstring_count   (const char *s);
s64   cstring_compare (const char *a, const char *b);
s64   cstring_compare (const char *a, const char *b, u64 n);

f32 Floor (f32 f);
f32 Ceil  (f32 f);
f32 Sqrt  (f32 f);
f32 Rsqrt (f32 f);
f32 Log2  (f32 f);
f32 Sin   (f32 r);
f32 Cos   (f32 r);
f32 Tan   (f32 r);

inline bool equal      (const String a, const String &b) { return cstring_compare((char *)a.data, (char *)b.data, Max(a.size, b.size)) == 0; }
inline bool operator== (const String a, const String &b) { return equal(a, b); }
inline bool operator!= (const String a, const String &b) { return !(a == b); }
inline s64  compare    (const String a, const String &b) { return cstring_compare((char *)a.data, (char *)b.data, Max(a.size, b.size)); }
inline bool operator<  (const String a, const String &b) { return compare(a, b) < 0; }

extern u64 get_current_thread_id ();

#ifndef TEMPORARY_STORAGE_ALIGNMENT
#define TEMPORARY_STORAGE_ALIGNMENT DEFAULT_ALIGNMENT
#endif

#ifndef TEMPORARY_STORAGE_DEFAULT_SIZE
#define TEMPORARY_STORAGE_DEFAULT_SIZE Megabytes(1)
#endif

#ifndef TEMPORARY_STORAGE_OVERFLOW_PAGE_SIZE
#define TEMPORARY_STORAGE_OVERFLOW_PAGE_SIZE Kilobytes(64)
#endif

void *__temporary_storage_allocator_proc (Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data);

struct Temporary_Storage {
    struct Overflow_Page {
        Overflow_Page *next = null;
        u64 size = 0;
    };

    u8 *original_data = null;
    u64 original_size = 0;
    
    u8 *data = null;
    u64 size = 0;
    u64 occupied = 0;
    u64 total_occupied = 0;
    u64 high_water_mark = 0;

    u64 total_size = 0; // only for stats
    
    Source_Code_Location last_set_mark_location;
    
    Allocator overflow_allocator;
    Overflow_Page *overflow_page = null;
};

void set_temporary_storage_mark (u64 mark, Source_Code_Location loc = __location);
u64  get_temporary_storage_mark ();
void reset_temporary_storage    ();

void *talloc (u64 size);

#ifndef ATOM_TABLE_DEFAULT_CAPACITY
#define ATOM_TABLE_DEFAULT_CAPACITY 512u
#endif

#ifndef ATOM_TABLE_DEFAULT_BUFFER_SIZE
#define ATOM_TABLE_DEFAULT_BUFFER_SIZE Kilobytes(128)
#endif

struct Atom {
    u64 hash = 0;
#if DEVELOPER
    String string;
#endif

    explicit operator bool() const {
#if DEVELOPER
        return hash && is_valid(string);
#else
        return hash;
#endif
    }
};
    
struct Atom_Table {
    Atom   *atoms   = null;
    String *strings = null;
    
    u32 count    = 0;
    u32 capacity = 0;

    Buffer buffer;
    u64    used = 0;
};

Atom   make_atom  (String s);
String get_string (Atom atom);

#define ATOM(literal) make_atom(S(literal))

inline Temporary_Storage __make_default_temporary_storage ();
inline thread_local auto __default_temporary_storage = __make_default_temporary_storage();

inline Atom_Table __make_default_atom_table ();
inline thread_local auto __default_atom_table = __make_default_atom_table();

struct Context {
    u64       thread_id = get_current_thread_id();
    Log_Level log_level = DEFAULT_LOG_LEVEL;

    Allocator   allocator   = __default_allocator;
    Logger      logger      = __default_logger;
    Assert_Proc assert_proc = __default_assert_proc;
    
    Temporary_Storage *temporary_storage = &__default_temporary_storage;
    Atom_Table        *atom_table        = &__default_atom_table;
};

inline thread_local Context context;

inline thread_local Allocator __temporary_allocator = {
    .proc = __temporary_storage_allocator_proc,
    .data = context.temporary_storage,
};

inline bool operator==(const Atom &a, const Atom &b) {
#if DEVELOPER
    if (a.hash == b.hash) {
        Assert(a.string == b.string);
        return true;
    }
    return false;
#else
    return a.hash == b.hash;
#endif
}

inline bool operator!=(const Atom &a, const Atom &b) { return !(a == b); }

template <typename T>
struct Array {    
    Allocator allocator = context.allocator;
    
    T  *items    = null;
    u32 count    = 0;
    u32 capacity = 0;

    T &operator[] (u32 index) const { Assert(index < count); return items[index]; }

    T *begin () const { return items; }
    T *end   () const { return items + count; }
};

template <typename T>
void array_clear(Array<T> &array) {
    array.count = 0;
    set(array.items, 0, array.capacity * sizeof(array.items[0]));
}

template <typename T>
void array_reset(Array<T> &array) {
    auto &allocator = array.allocator;
    allocator.proc(FREE, 0, array.capacity * sizeof(T), array.items, allocator.data);

    array.items    = null;
    array.count    = 0;
    array.capacity = 0;
}

template <typename T>
void array_realloc(Array<T> &array, u32 new_capacity) {
    if (new_capacity <= array.capacity) return;

    auto old_items = array.items;
    array.items = (T *)array.allocator.proc(RESIZE, new_capacity * sizeof(T), array.capacity * sizeof(T), old_items, array.allocator.data);
    array.capacity = new_capacity;
}

template <typename T>
T *array_find(Array<T> &array, const T &item) {
    for (s32 i = 0; i < (s32)array.count; ++i) {
        auto &current = array[i];
        if (current == item) return &current;
    }
    
    return null;
}

template <typename T>
T &array_add(Array<T> &array) {
    if (array.count >= array.capacity) {
        array_realloc(array, array.capacity * 2 + 1);
    }
    
    auto &item = array.items[array.count];
    array.count += 1;
    
    return item;
}

template <typename T>
T &array_add(Array<T> &array, const T &item) {
    auto t = new (&array_add(array)) T {item};
    return *t;
}

template <typename T>
s32 array_remove_swap(Array<T> &array, const T &item) {
    s32 index = INDEX_NONE;
    for (s32 i = 0; i < (s32)array.count; ++i) {
        auto &current = array[i];
        if (current == item) {
            current = array_pop(array);
            index = i;
        }
    }
    
    return index;
}

template <typename T>
T &array_pop(Array<T> &array) {
    Assert(array.count > 0);
    auto &item = array[array.count - 1];
    array.count -= 1;
    return item;
}

template <typename T>
T &array_last(Array<T> &array) {
    Assert(array.count > 0);
    return array[array.count - 1];
}

enum String_Op_Bits : u32 {
    S_SEARCH_REVERSE_BIT  = 0x1,
    S_INDEX_PLUS_ONE_BIT  = 0x2,
    S_INDEX_MINUS_ONE_BIT = 0x4,
    S_LENGTH_ON_FAIL_BIT  = 0x8,
    S_START_PLUS_ONE_BIT  = 0x10,
    S_LEFT_SLICE_BIT      = 0x20,
};

#define S(literal) String { .data = (u8 *)literal, .size = sizeof(literal) - 1 }

inline String make_string (u8 *s)              { return { s, cstring_count((char *)s) }; }
inline String make_string (u8 *s, u64 count)   { return { s, count }; }
inline String make_string (char *s)            { return { (u8 *)s, cstring_count(s) }; }
inline String make_string (char *s, u64 count) { return { (u8 *)s, count }; }
inline String make_string (String s)           { return { s.data, s.size }; }
inline String make_string (Buffer b)           { return { b.data, b.size }; }

String copy_string   (String s, Allocator alc = context.allocator);
String copy_string   (const u8 *s, Allocator alc = context.allocator);
String copy_string   (const u8 *data, u64 count, Allocator alc = context.allocator);
char  *to_c_string   (String s, Allocator alc);
char  *temp_c_string (String s);

inline String copy_string(const char *s, Allocator alc = context.allocator) {
    return copy_string((u8 *)s, cstring_count(s), alc);
}

inline String copy_string(const char *data, u64 count, Allocator alc = context.allocator) {
    return copy_string((u8 *)data, count, alc);
}

String trim  (String s);
String slice (String s, s64 start);
String slice (String s, s64 start, s64 end);
String slice (String s, char   c,   u32 bits = 0);
String slice (String s, String sub, u32 bits = 0);

s64 find (String s, char   c,   u32 bits = 0);
s64 find (String s, String sub, u32 bits = 0);

s64 string_to_integer (String s);
f64 string_to_float   (String s);

Array <String> split (String s, String delims, Allocator alc = __temporary_allocator);
Array <String> chop  (String s, u64 chop_size, Allocator alc = __temporary_allocator);

inline constexpr String LOG_IDENT_NONE = {};

void __push_context (const Context &ctx);
void __pop_context  ();

#define push_context(ctx) for (s32 _flag = (__push_context(ctx), 0); !_flag; __pop_context(), _flag = 1)

void __push_allocator (const Allocator &allocator);
void __pop_allocator  ();

#define push_allocator(alc) for (s32 _flag = (__push_allocator(alc), 0); !_flag; __pop_allocator(), _flag = 1)

void *alloc   (u64   size,                         Allocator alc = context.allocator);
void *resize  (void *data, u64 size,               Allocator alc = context.allocator);
void *resize  (void *data, u64 size, u64 old_size, Allocator alc = context.allocator);
void  release (void *data,                         Allocator alc = context.allocator);

#define __get_new_macro(_1, _2, _3, name, ...) name
#define __new1(T)       new (alloc((1) * sizeof(T))) T
#define __new2(T, N)    new (alloc((N) * sizeof(T))) T[N]
#define __new3(T, N, A) new (alloc((N) * sizeof(T), A)) T[N]
#define New(...) __expand(__get_new_macro(__VA_ARGS__, __new3, __new2, __new1)(__VA_ARGS__))

#define __get_delete_macro(_1, _2, name, ...) name
#define __delete1(P)    release(P)
#define __delete2(P, A) release(P, A)
#define Delete(...) __expand(__get_delete_macro(__VA_ARGS__, __delete2, __delete1)(__VA_ARGS__))

#define push_va_args(format)                                                 \
    for (bool _va_scope = true; _va_scope; _va_scope = false)                \
        for (va_list va_args; _va_scope; _va_scope = false, va_end(va_args)) \
            for (va_start(va_args, format); _va_scope; _va_scope = false)

void   print     (String message);
void   print     (const char *format, ...);
void   log       (const char *format, ...);
void   log       (Log_Level level, const char *format, ...);
void   log       (String    ident, const char *format, ...);
void   log       (String    ident, Log_Level level, const char *format, ...);
void   log_va    (String    ident, Log_Level level, const char *format, va_list args);
String sprint    (const char *format, ...);
String tprint    (const char *format, ...);
String tprint_va (const char *format, va_list args);
