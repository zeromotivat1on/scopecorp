#include "pch.h"
#include "pak.h"
#include "hash.h"
#include "archive.h"
#include "hash_table.h"
#include "string_builder.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#undef STB_SPRINTF_IMPLEMENTATION

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <intrin.h>

void set_bit    (void *p, u64 n)       { ((u8 *)p)[n / 8] |=  (1ull << (n % 8)); }
void clear_bit  (void *p, u64 n)       { ((u8 *)p)[n / 8] &= ~(1ull << (n % 8)); }
void toggle_bit (void *p, u64 n)       { ((u8 *)p)[n / 8] ^=  (1ull << (n % 8)); }
bool check_bit  (const void *p, u64 n) { return (((u8 *)p)[n / 8] & (1ull << (n % 8))) != 0; }

void __and (void *r, const void *a, const void *b, u64 n) { for (u64 i = 0; i < n; ++i) ((u8 *)r)[i] = ((u8 *)a)[i] & ((u8 *)b)[i]; }
void __or  (void *r, const void *a, const void *b, u64 n) { for (u64 i = 0; i < n; ++i) ((u8 *)r)[i] = ((u8 *)a)[i] | ((u8 *)b)[i]; }
void __xor (void *r, const void *a, const void *b, u64 n) { for (u64 i = 0; i < n; ++i) ((u8 *)r)[i] = ((u8 *)a)[i] ^ ((u8 *)b)[i]; }
void __not (void *r, const void *a, u64 n)                { for (u64 i = 0; i < n; ++i) ((u8 *)r)[i] = ~((u8 *)a)[i]; }

u128 operator& (const u128 &a, const u128 &b) { u128 r; __and(&r, &a, &b, sizeof(r)); return r; }
u256 operator& (const u256 &a, const u256 &b) { u256 r; __and(&r, &a, &b, sizeof(r)); return r; }
u512 operator& (const u512 &a, const u512 &b) { u512 r; __and(&r, &a, &b, sizeof(r)); return r; }
u128 operator| (const u128 &a, const u128 &b) { u128 r; __or (&r, &a, &b, sizeof(r)); return r; }
u256 operator| (const u256 &a, const u256 &b) { u256 r; __or (&r, &a, &b, sizeof(r)); return r; }
u512 operator| (const u512 &a, const u512 &b) { u512 r; __or (&r, &a, &b, sizeof(r)); return r; }
u128 operator^ (const u128 &a, const u128 &b) { u128 r; __xor(&r, &a, &b, sizeof(r)); return r; }
u256 operator^ (const u256 &a, const u256 &b) { u256 r; __xor(&r, &a, &b, sizeof(r)); return r; }
u512 operator^ (const u512 &a, const u512 &b) { u512 r; __xor(&r, &a, &b, sizeof(r)); return r; }
u128 operator~ (const u128 &a)                { u128 r; __not(&r, &a,     sizeof(r)); return r; }
u256 operator~ (const u256 &a)                { u256 r; __not(&r, &a,     sizeof(r)); return r; }
u512 operator~ (const u512 &a)                { u512 r; __not(&r, &a,     sizeof(r)); return r; }

void *eat(void **data, u64 n) {
    void *p = *data;
    *data = (u8 *)(*data) + n;
    return p;
}

// Hint compiler to not reorder memory operations, happened before barrier.
// Basically disable memory read/write concerned optimizations.
void read_barrier()   { _ReadBarrier(); }
void write_barrier()  { _WriteBarrier(); }
void memory_barrier() { _ReadWriteBarrier(); }

// Ensure the order of read/write CPU instructions.
void read_fence()   { _mm_lfence(); }
void write_fence()  { _mm_sfence(); }
void memory_fence() { _mm_mfence(); }

void set       (void *p, s32 v, u64 n)             { memset(p, v, n); }
void copy      (void *dst, const void *src, u64 n) { memcpy(dst, src, n); }
void safe_copy (void *dst, const void *src, u64 n) { memmove(dst, src, n); }

char *cstring_copy (const char *s) { return cstring_copy(s, context.allocator); }

char *cstring_copy(const char *s, Allocator alc) {
    auto count = strlen(s);
    char *new_s = (char *)alloc(count + 1, alc);
    copy(new_s, s, count);
    return new_s;
}

u64 cstring_count(const char *s) {
    u64 n = 0;
    while(s[n++]);
    return n - 1;
}

s64 cstring_compare(const char *a, const char *b) {
    while (*a && (*a == *b)) { ++a; ++b; }
    return (*(u8 *)a - *(u8 *)b);
}

s64 cstring_compare(const char *a, const char *b, u64 n) {
    while (n && *a && (*a == *b)) { ++a; ++b; --n; }
    return n == 0 ? 0 : (*(u8 *)a - *(u8 *)b);
}

f32 Floor(f32 f) {
#if 0
    f32 t = (f32) (int) value;
    return (t != f) ? (t - 1.0f) : t;
#else
    return floorf(f);
#endif
}

f32 Ceil(f32 f) {
#if 0
    u32 input;
    copy(&input, &f, 4);

    const s32 exponent = ((input >> 23) & 255) - 127;
    if (exponent < 0) return (f > 0); // small numbers get rounded to 0 or 1, depending on their sign

    const s32 fractional_bits = 23 - exponent;
    if (fractional_bits <= 0) return f; // numbers without fractional bits are mapped to themselves

    // Round the number down by masking out the fractional bits.
    const u32 integral_mask = 0xffffffff << fractional_bits;
    const u32 output        = input & integral_mask;

    memcpy(&f, &output, 4);
    if (f > 0 && output != input) f += 1; // positive numbers need to be rounded up, not down

    return f;
#else
    return ceilf(f);
#endif
}

f32 Sqrt(f32 f) { return f * Rsqrt(f); }

f32 Rsqrt(f32 f) {    
	const auto y = f * 0.5f;
    
	auto i = *(s32 *)&f;
	i = 0x5f3759df - (i >> 1);
    
	auto r = *(f32 *)&i;
	r = r * (1.5f - r * r * y);
    
	return r;
}

f32 Log2(f32 f) {
    return log2f(f);
}

f32 Sin (f32 r) { return sinf(r); }
f32 Cos (f32 r) { return cosf(r); }
f32 Tan (f32 r) { return tanf(r); }

void *__default_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data) {
    switch (mode) {
    case ALLOCATE: {
        auto data = malloc(size);
        set(data, 0, size);
        return data;
    }
    case RESIZE:   {
        auto data = realloc(old_memory, size);
        if (old_size && size > old_size) {
            set((u8 *)data + old_size, 0, size - old_size);
        }
        return data;
    }
    case FREE:     { free(old_memory); return null; }
    case FREE_ALL: { return null; }
    default:       { unreachable_code_path(); return null; }
    }
}

void __default_logger_proc(String message, String ident, Log_Level level, void *logger_data) {
    if (level < context.log_level) return;
    
    if (ident) {
        print("[%S] %S\n", ident, message);
    } else {
        print("%S\n", message);
    }
}

void *__temporary_storage_allocator_proc (Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data) {
    using Overflow_Page = Temporary_Storage::Overflow_Page;
    
    constexpr auto alignment          = TEMPORARY_STORAGE_ALIGNMENT;
    constexpr auto overflow_page_size = TEMPORARY_STORAGE_OVERFLOW_PAGE_SIZE;
    
    Assert(allocator_data);

    auto &storage = *(Temporary_Storage *)allocator_data;
    Assert(storage.data);
    Assert(storage.size);

    size        = Align(size, alignment);
    auto offset = Align(storage.occupied, alignment);
        
    switch (mode) {
    case RESIZE:
    case ALLOCATE: {
        // @Cleanup: this overflow handling is not that good memory-wise as it simply
        // allocates new page if current size is not big enough. But maybe thats fine
        // and reasonable tradeoff to skip search for free space in overflow page list.
        if (offset + size > storage.size) {
            auto overflow_size = Max(overflow_page_size + sizeof(Overflow_Page), size + sizeof(Overflow_Page));
                 overflow_size = Align(overflow_size, alignment);
            
            auto &allocator = storage.overflow_allocator;
            auto memory = (u8 *)allocator.proc(ALLOCATE, overflow_size, 0, null, allocator.data);

            auto new_page  = (Overflow_Page *)memory;
            new_page->next = null;
            new_page->size = overflow_size - sizeof(Overflow_Page);

            if (!storage.overflow_page) {
                storage.overflow_page = new_page;
            } else {
                auto page = storage.overflow_page;
                while (page->next) page = page->next;
                page->next = new_page;
            }
            
            storage.data = memory + sizeof(Overflow_Page);
            storage.size = new_page->size;
            storage.occupied = 0;
            storage.total_size += new_page->size;
            
            offset = 0;
        }

        Assert(offset + size <= storage.size); // sanity check
        
        auto data = storage.data + offset;
        set(data, 0, size);
        
        storage.occupied       += size;
        storage.total_occupied += size;
                    
        if (storage.high_water_mark < storage.total_occupied)
            storage.high_water_mark = storage.total_occupied;

        if (mode == RESIZE && old_memory && old_size) {
            Assert(old_size < size);
            copy(data, old_memory, old_size);
        }
        
        return data;
    }
    case FREE: {
        return null;
    }
    case FREE_ALL: {
        auto page = storage.overflow_page;
        while (page) {
            auto &allocator = storage.overflow_allocator;
            auto next = page->next;
            allocator.proc(FREE, 0, 0, page, allocator.data);
            page = next;
        }

        storage.data = storage.original_data;
        storage.size = storage.original_size;
        storage.occupied = 0;
        storage.total_occupied = 0;
        storage.total_size     = storage.original_size;
        storage.overflow_page = null;
        storage.last_set_mark_location = __location;

        return null;
    }
    default: {
        unreachable_code_path();
        return null;
    }
    }
}

Temporary_Storage __make_default_temporary_storage() {
    Temporary_Storage ts = {};
    ts.original_size = TEMPORARY_STORAGE_DEFAULT_SIZE;
    // @Todo: fix this, as we are using malloc here now.
    // Here we are using platform heap alloc instead of crt as using malloc causes crash
    // when attaching from RenderDoc, probably due to crt is not initialized at the moment.
    //
    // The assumption above was kinda on the right way, the problem was with static crt
    // linking with an app that uses global thread_local variables that during initialization
    // may involve calls to crt systems like malloc. Dynamic crt linking fixes the issue.
    ts.original_data = (u8 *)malloc(ts.original_size);
    ts.size = ts.original_size;
    ts.data = ts.original_data;
    ts.total_size = ts.original_size;
    ts.overflow_allocator = __default_allocator;
    return ts;
}

Atom_Table __make_default_atom_table() {    
    Atom_Table t = {};
    t.capacity = ATOM_TABLE_DEFAULT_CAPACITY;
    t.atoms    = (Atom *)  malloc(t.capacity * sizeof(Atom));
    t.strings  = (String *)malloc(t.capacity * sizeof(String));
    t.buffer.size = ATOM_TABLE_DEFAULT_BUFFER_SIZE;
    t.buffer.data = (u8 *)malloc(ATOM_TABLE_DEFAULT_BUFFER_SIZE);

    set(t.atoms,       0, t.capacity * sizeof(Atom));
    set(t.strings,     0, t.capacity * sizeof(String));
    set(t.buffer.data, 0, ATOM_TABLE_DEFAULT_BUFFER_SIZE);
    
    return t;
}

Atom make_atom(String s) {
    auto table = context.atom_table;
    auto hash  = hash_fnv(s);
    auto index = hash % table->capacity;

    // We merely do not believe in atom collision.
    Assert(index < table->capacity);
    
    auto atom   = &table->atoms[index];    
    auto string = &table->strings[index];

    if (atom->hash) {
        Assert(atom->string == *string);
        return *atom;
    }
    
    Assert(!string->data && !string->size);
    Assert(table->used + s.size <= table->buffer.size);
    
    auto data = table->buffer.data + table->used;
    copy(data, s.data, s.size);

    table->count += 1;
    table->used  += s.size;
    
    string->data  = data;
    string->size = s.size;

    atom->hash = hash;
#if DEVELOPER
    atom->string = *string;
#endif
    
    return *atom;
}

String get_string(Atom atom) {
    auto table = context.atom_table;
    auto index = atom.hash % table->capacity;

    auto string = table->strings[index];
    Assert(string);

#if DEVELOPER
    Assert(string == atom.string);
#endif
    
    return string;
}

void set_temporary_storage_mark(u64 mark, Source_Code_Location loc) {
    auto storage = context.temporary_storage;
    Assert(mark <= storage->total_occupied);

    auto page = storage->overflow_page;
    while (page) {
        if (mark < page->size) break;
        mark -= page->size;
        page = page->next;
    }
    
    storage->occupied = mark;
    storage->last_set_mark_location = loc;
}

u64  get_temporary_storage_mark () { return context.temporary_storage->total_occupied; }
void reset_temporary_storage    () { __temporary_allocator.proc(FREE_ALL, 0, 0, null, __temporary_allocator.data); }
void *talloc (u64 size) { return alloc(size, __temporary_allocator); }

void __default_assert_proc(const char *condition, Source_Code_Location loc, const char *format, ...) {
#if DEVELOPER
    static char log_buffer[1024];

    String message;
    message.data = (u8 *)log_buffer;
    message.size = 0;
    
    if (format) {
        static char format_buffer[1024];

        push_va_args(format) {
            stbsp_vsnprintf(format_buffer, carray_count(format_buffer), format, va_args);
        }

        message.size = stbsp_snprintf(log_buffer, carray_count(log_buffer), "Assertion failed (%s). %s\n", condition, format_buffer);
    } else {
        message.size = stbsp_snprintf(log_buffer, carray_count(log_buffer), "Assertion failed (%s)\n", condition);
    }

    fwrite(message.data, 1, message.size, stdout);
    
    auto callstack = get_current_callstack();
    For (callstack) {
        message.size = stbsp_snprintf(log_buffer, carray_count(log_buffer), "%s:%s:%d\n", it.file, it.function, it.line);
        fwrite(message.data, 1, message.size, stdout);
    }
    
#ifdef _MSC_VER
    __debugbreak();
#endif
#endif
}

thread_local Context __ctx_stack[8];
thread_local u32     __ctx_size = 0;

void __push_context(const Context &ctx) {
    Assert(__ctx_size < carray_count(__ctx_stack));
    __ctx_stack[__ctx_size] = context;
    __ctx_size += 1;
    context = ctx;
}

void __pop_context() {
    Assert(__ctx_size > 0);
    __ctx_size -= 1;
    context = __ctx_stack[__ctx_size];
}

thread_local Allocator __alc_stack[2 * carray_count(__ctx_stack)];
thread_local u32       __alc_size = 0;

void __push_allocator(const Allocator &allocator) {
    Assert(__alc_size < carray_count(__alc_stack));
    __alc_stack[__alc_size] = context.allocator;
    __alc_size += 1;
    context.allocator = allocator;
}

void __pop_allocator() {
    Assert(__alc_size > 0);
    __alc_size -= 1;
    context.allocator = __alc_stack[__alc_size];
}

void *alloc   (u64 size, Allocator alc)                           { return alc.proc(ALLOCATE, size, 0, null, alc.data); }
void *resize  (void *data, u64 size, Allocator alc)               { return alc.proc(RESIZE, size, 0, data, alc.data); }
void *resize  (void *data, u64 size, u64 old_size, Allocator alc) { return alc.proc(RESIZE, size, old_size, data, alc.data); }
void  release (void *data, Allocator alc)                         { alc.proc(FREE, 0, 0, data, alc.data); }

String copy_string (String s, Allocator alc)    { return copy_string(s.data, s.size, alc); }
String copy_string (const u8 *s, Allocator alc) { return copy_string(s, cstring_count((char *)s), alc); }

String copy_string(const u8 *data, u64 size, Allocator alc) {
    String s;
    s.size = size;
    s.data  = (u8 *)alloc(size, alc);
    copy(s.data, data, size);
    return s;
}

char *to_c_string(String s, Allocator alc) {
    char *buffer = (char *)alloc(s.size + 1, alc);
    copy(buffer, s.data, s.size);
    buffer[s.size] = '\0';
    return buffer;
}

char *temp_c_string(String s) { return to_c_string(s, __temporary_allocator); }

String trim(String s) {
    while (Is_Space(*(s.data)))               { s.data  += 1; }
    while (Is_Space(*(s.data + s.size - 1))) { s.size -= 1; }
    return s;
}

String slice(String s, s64 start) {
    return slice(s, start, s.size - 1);
}

String slice(String s, s64 start, s64 end) {
    u64 ustart = Abs(start);
    u64 uend   = Abs(end);

    if (ustart >= s.size) return {};
    if (uend   >= s.size) return {};

    if (start < 0) ustart = s.size - ustart;
    if (end   < 0) uend   = s.size - uend;

    s.data  = s.data + ustart;
    s.size = uend   - ustart;
    
    return s;
}

String slice(String s, char c, u32 bits) {
    u8 a = c;
    return slice(s, make_string(&a, 1), bits);
}

String slice(String s, String sub, u32 bits) {
    const auto index = find(s, sub, bits);
    if (index < 0 || index > (s64)s.size) return {};
    
    if (bits & S_LEFT_SLICE_BIT) {
        s.size = index + 1;
    } else {
        s.data  = s.data  + index;
        s.size = s.size - index;
    }

    return s;
}

Array <String> split(String s, String delims, Allocator alc) {
    Array <String> tokens;
    tokens.allocator = alc;
    
    u64 start = 0;
    while (1) {
        while (start < s.size && find(delims, s.data[start]) != INDEX_NONE) start += 1;
    
        if (start >= s.size) break;
    
        auto end = start;
        while (end < s.size && find(delims, s.data[end]) == INDEX_NONE) end += 1;

        const auto token = make_string(s.data + start, end - start);
        array_add(tokens, token);

        start = end + 1;
    }
    
    return tokens;
}

Array <String> chop(String s, u64 chop_size, Allocator alc) {
    Array <String> tokens;
    tokens.allocator = alc;
    
    u64 start = 0;
    while (1) {
        if (start + chop_size >= s.size) {
            const auto token = make_string(s.data + start, s.size - start);
            array_add(tokens, token);
            break;
        }

        const auto token = make_string(s.data + start, chop_size);
        array_add(tokens, token);
            
        start += chop_size;
    }
    
    return tokens;
}

s64 find(String s, char c, u32 bits) {
    u8 a = c;
    return find(s, String { &a, 1 }, bits);
}

s64 find(String s, String sub, u32 bits) {
    if (sub.size == 0)      return 0;
    if (sub.size > s.size) return INDEX_NONE;

    s64 index = INDEX_NONE;

    if (bits & S_SEARCH_REVERSE_BIT) {
        s64 start = s.size - 1;
        // @Todo: should we ignore S_START_PLUS_ONE_BIT or handle and if so how?

        for (s64 i = start - sub.size; i >= 0; --i) {
            bool match = true;
            for (s64 j = sub.size - 1; j >= 0; --j) {
                if (s.data[i + j] != sub.data[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                index = i;
                break;
            }
        }
    } else {
        s64 start = 0;
        if (bits & S_START_PLUS_ONE_BIT) start += 1;

        for (s64 i = start; i + sub.size <= s.size; ++i) {
            bool match = true;
            for (u64 j = 0; j < sub.size; ++j) {
                if (s.data[i + j] != sub.data[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                index = i;
                break;
            }
        }
    }
    
    if (index == INDEX_NONE) {
        if (bits & S_LENGTH_ON_FAIL_BIT) index = s.size;
    } else {
        if (bits & S_INDEX_PLUS_ONE_BIT)  index += 1;
        if (bits & S_INDEX_MINUS_ONE_BIT) index -= 1;
    }
    
    return index;
}

s64 string_to_integer(String s) {
    auto buffer = temp_c_string(s);
    return strtol(buffer, null, 0);
}

f64 string_to_float(String s) {
    auto buffer = temp_c_string(s);
    return strtod(buffer, null);
}

void print(String message) {
    fwrite(message.data, 1, message.size, stdout);
}

void print(const char *format, ...) {
    thread_local char buffer[4096];
	push_va_args(format) {
        const auto size = stbsp_vsnprintf(buffer, carray_count(buffer), format, va_args);
        fwrite(buffer, 1, size, stdout);
    }
}

void log(const char *format, ...) {
    push_va_args(format) { log_va(LOG_IDENT_NONE, LOG_DEFAULT, format, va_args); }
}

void log(Log_Level level, const char *format, ...) {
    push_va_args(format) { log_va(LOG_IDENT_NONE, level, format, va_args); }
}

void log(String ident, const char *format, ...) {
    push_va_args(format) { log_va(ident, LOG_DEFAULT, format, va_args); }
}

void log(String ident, Log_Level level, const char *format, ...) {
    push_va_args(format) { log_va(ident, level, format, va_args); }
}

void log_va(String ident, Log_Level level, const char *format, va_list args) {
    thread_local char buffer[4096];

    String message;
    message.data = (u8 *)buffer;
    message.size = stbsp_vsnprintf(buffer, carray_count(buffer), format, args);
    
    context.logger.proc(message, ident, level, context.logger.data);
}

String sprint(const char *format, ...) {
    String_Builder builder;
    push_va_args(format) { print_to_builder_va(builder, format, va_args); }
    return builder_to_string(builder);
}

String tprint(const char *format, ...) {
    String s;
    push_va_args(format) { s = tprint_va(format, va_args); }
    return s;
}

String tprint_va(const char *format, va_list args) {
    String_Builder builder;
    builder.allocator = __temporary_allocator;
    print_to_builder_va(builder, format, args);
    return builder_to_string(builder);
}

u32 hash_pcg(u32 input) {
    const u32 state = input * 747796405u + 2891336453u;
    const u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u64 hash_fnv(String s) {
    constexpr u64 FNV_BASIS = 14695981039346656037;
    constexpr u64 FNV_PRIME = 1099511628211;

    u64 hash = FNV_BASIS;
    for (u64 i = 0; i < s.size; ++i) {
        hash ^= (u64)(u8)(s.data[i]);
        hash *= FNV_PRIME;
    }

    return hash;
}

void sort(void *data, u32 count, u32 size, s32 (*compare)(const void *, const void *)) {
    qsort(data, count, size, compare);
}

String get_build_type_name() {
#if DEBUG
    return S("DEBUG");
#elif RELEASE
    return S("RELEASE");
#else
#error "Unknown build type"
#endif
}

String get_file_name_no_ext(String path) {
    s64 start = find(path, '/', S_SEARCH_REVERSE_BIT | S_INDEX_PLUS_ONE_BIT);
    if (start == INDEX_NONE) start = 0;

    s64 end = find(path, '.', S_SEARCH_REVERSE_BIT);
    if (end == INDEX_NONE) {
        if (start == 0) return path;
        end = path.size - 1;
    }

    return slice(path, start, end);
}

String get_extension(String path) { return slice(path, '.', S_INDEX_PLUS_ONE_BIT); }

static String_Builder_Node *add_node(String_Builder &builder) {
    auto new_node = (String_Builder_Node *)builder.allocator.proc(ALLOCATE, sizeof(String_Builder_Node), 0, null, builder.allocator.data);
    
    if (!builder.node) {
        builder.node = new_node;
    } else {
        auto node = builder.node;
        while (node->next) node = node->next;
        node->next = new_node;
    }
    
    return new_node;
}

void append(String_Builder &builder, const void *data, u64 size) {
    auto node = add_node(builder);

    node->data = (u8 *)builder.allocator.proc(ALLOCATE, size, 0, null, builder.allocator.data);
    node->size = size;

    copy(node->data, data, size);
}

void append (String_Builder &builder, const char *s)           { append(builder, s, strlen(s)); }
void append (String_Builder &builder, const char *s, u64 size) { append(builder, (const u8 *)s, size); }
void append (String_Builder &builder, char c)                  { append(builder, &c, 1); }
void append (String_Builder &builder, Buffer buffer)           { append(builder, buffer.data, buffer.size); }
void append (String_Builder &builder, String string)           { append(builder, string.data, string.size); }

void print_to_builder(String_Builder &builder, const char *format, ...) {    
    push_va_args(format) print_to_builder_va(builder, format, va_args);
}

void print_to_builder_va(String_Builder &builder, const char *format, va_list args) {
    // @Cleanup @Speed @Robustness: intermediate buffer allows us to be memory
    // efficient in terms of string builder allocations, but its slower due to 
    // extra memory copy.
    
    constexpr u32 size = Kilobytes(4);
    thread_local char buffer[size];

    String s;
	s.size = stbsp_vsnprintf(buffer, size, format, args);
    s.data  = (u8 *)builder.allocator.proc(ALLOCATE, s.size, 0, null, builder.allocator.data);
    copy(s.data, buffer, s.size);

    auto node = add_node(builder);
    node->data = s.data;
    node->size = s.size;
}

static inline u64 get_total_size(const String_Builder &builder) {
    u64  size = 0;
    auto node = builder.node;
    while (node) { size += node->size; node = node->next; }
    return size;
}

String builder_to_string(String_Builder &builder) {
    return builder_to_string(builder, builder.allocator);
}

String builder_to_string(String_Builder &builder, Allocator allocator) {
    const auto size = get_total_size(builder);
    
    String s;
    s.data = (u8 *)alloc(size, allocator);

    auto data = s.data;
    auto node = builder.node;
    
    while (node) {
        copy(data, node->data, node->size);

        s.size += node->size;
        data   += node->size;

        release(node->data, allocator);
        
        auto old_node = node;
        node = node->next;

        release(old_node, allocator);
    }

    Assert(s.size == size);
    
    builder.node = null;

    return s;
}

void add(Create_Pak &pak, String entry_name, Buffer entry_buffer, u64 user_value) {
    auto &entry  = array_add(pak.entries);
    entry.name   = entry_name;
    entry.buffer = entry_buffer;
    entry.user_value = user_value;
    
    // We don't compute file offset here. It's computed during write to
    // allow the client to sort entries after all of them were added.
}

bool write(Create_Pak &pak, String path) {
    Archive archive;
    defer { reset(archive); };
    
    if (!start_write(archive, path)) return false;
    
    u64 offset_from_file_start = sizeof(Pak_Header);
    
    // Precompute entry offsets, so we can deduce table of contents offset as well.
    For (pak.entries) {
        it.offset_from_file_start = offset_from_file_start;
        offset_from_file_start += it.buffer.size;
    }
    
    Pak_Header header;
    header.magic   = PAK_MAGIC;
    header.version = PAK_VERSION;
    header.bits    = 0;
    header.table_of_contents_offset = offset_from_file_start;

    serialize(archive, header);
    
    u64 check_offset = sizeof(Pak_Header);
    For (pak.entries) {
        Assert(check_offset == it.offset_from_file_start);
        
        if (!serialize(archive, it.buffer.data, it.buffer.size)) {
            log(LOG_ERROR, "Failed to write entry %S buffer data to %S", it.name, path);
            return false;
        }
        
        check_offset += it.buffer.size;
    }

    String_Builder builder;
    builder.allocator = __temporary_allocator;
    
    Pak_Toc toc;
    toc.magic = PAK_TOC_MAGIC;
    toc.entry_count = pak.entries.count;

    put(builder, toc);
    
    For (pak.entries) {
        if (it.name.size > it.MAX_NAME_LENGTH) {
            log(LOG_ERROR, "Pak entry name length is too long or corrupted (max length is %llu)", it.MAX_NAME_LENGTH);
            return false;
        }

        // Compute entry read size, see pak.h for pak format specification.
        const auto entry_read_size = sizeof(u16) + it.name.size + sizeof(u32) + 2 * sizeof(u64);
        Assert(entry_read_size <= it.MAX_READ_SIZE);
        
        put   (builder, (u16)entry_read_size);
        put   (builder, (u16)it.name.size);
        append(builder, it.name.data, it.name.size);
        put   (builder, (u32)it.buffer.size);
        put   (builder, it.user_value);
        put   (builder, it.offset_from_file_start);
    }

    const auto s = builder_to_string(builder);
    if (!serialize(archive, s.data, s.size)) {
        log(LOG_ERROR, "Failed to write table of contents to pak %S", path);
        return false;
    }
    
    return true;
}

bool load(Load_Pak &pak, String path) {
    Archive archive;
    defer { reset(archive); };
    
    if (!start_read(archive, path)) return false;
    
    const u64 file_size = get_current_size(archive);
    if (file_size < sizeof(Pak_Header)) {
        log(LOG_ERROR, "Pak %S is too small to even contain header, it's %llu bytes", path, file_size);
        return false;
    }

    serialize(archive, pak.header);

    if (pak.header.magic != PAK_MAGIC) {
        log(LOG_ERROR, "Invalid pak file magic %u in %S, expected %u", pak.header.magic, path, PAK_MAGIC);
        return false;
    }

    if (pak.header.version > PAK_VERSION) {
        log(LOG_ERROR, "Invalid or newer version %d in pak %S, expected %u", pak.header.version, path, PAK_VERSION);
        return false;
    }

    if (pak.header.table_of_contents_offset > file_size) {
        log(LOG_ERROR, "Invalid table of contents offset %llu in pak %S of size %llu", pak.header.table_of_contents_offset, path, file_size);
        return false;
    }

    if (!set_pointer(archive, pak.header.table_of_contents_offset)) {
        log(LOG_ERROR, "Failed to set file pointer to %llu of pak %S", pak.header.table_of_contents_offset, path);
        return false;
    }

    serialize(archive, pak.toc);

    if (pak.toc.magic != PAK_TOC_MAGIC) {
        log(LOG_ERROR, "Invalid pak table of contents magic %u in %S at offset %llu, expected %u", pak.header.magic, path, pak.header.table_of_contents_offset, PAK_TOC_MAGIC);
        return false;
    }

    // @Speed: determine more-or-less optimal size to read pak
    // file by medium-large chunks instead of by each entry.

    void *entry_read_data = talloc(Pak_Entry::MAX_READ_SIZE);
    array_realloc(pak.entries, pak.toc.entry_count);
    
    for (u32 i = 0; i < pak.toc.entry_count; ++i) {
        u16 entry_read_size = 0;
        serialize(archive, entry_read_size);

        auto &entry = array_add(pak.entries);
        if (entry_read_size > entry.MAX_READ_SIZE) {
            log(LOG_ERROR, "Invalid entry #%u read size %u, max possible size is %u", i, entry_read_size, entry.MAX_READ_SIZE);
            return false;
        }

        serialize(archive, entry_read_data, entry_read_size);

        auto buffer = entry_read_data;
        entry.name.size = *Eat(&buffer, u16);
        entry.name.data = (u8 *)alloc(entry.name.size, pak.entry_data_allocator);
        copy(entry.name.data, eat(&buffer, entry.name.size), entry.name.size);

        entry.buffer.size = *Eat(&buffer, u32);
        
        entry.user_value             = *Eat(&buffer, u64);
        entry.offset_from_file_start = *Eat(&buffer, u64);

        if (entry.offset_from_file_start >= pak.header.table_of_contents_offset) {
            log(LOG_ERROR, "Invalid entry #%d data offset %llu, the data should be before table of contetns which offset is %llu", i, entry.offset_from_file_start, pak.header.table_of_contents_offset);
            return false;
        }
    }

    for (u32 i = 0; i < pak.toc.entry_count; ++i) {
        auto &entry = pak.entries[i];

        set_pointer(archive, entry.offset_from_file_start);

        entry.buffer.data = (u8 *)alloc(entry.buffer.size, pak.entry_data_allocator);
        serialize(archive, entry.buffer.data, entry.buffer.size);

        pak.lookup[entry.name] = &entry;
    }
    
    return true;
}

Pak_Entry *find_entry(Load_Pak &pak, String name) {
    if (auto found = table_find(pak.lookup, name)) return *found;
    return null;
}

// archive

bool start_read(Archive &archive, String path) {
    File file = open_file(path, FILE_READ_BIT);
    if (file == FILE_NONE) {
        log(LOG_ERROR, "Failed to start archive 0x%X read from %S", &archive, path);
        return false;
    }

    archive.mode = ARCHIVE_READ;
    archive.file = file;

    return true;
}

bool start_write(Archive &archive, String path) {
    File file = open_file(path, FILE_WRITE_BIT, false);
    if (file == FILE_NONE) {
        file = open_file(path, FILE_NEW_BIT | FILE_WRITE_BIT);
        if (file == FILE_NONE) {
            log(LOG_ERROR, "Failed to start archive 0x%X write to %S", &archive, path);
            return false;
        }
    }

    archive.mode = ARCHIVE_WRITE;
    archive.file = file;

    return true;
}

bool set_pointer(Archive &archive, u64 position) {
    return set_file_ptr(archive.file, position);
}

u64 get_current_size(Archive &archive) {
    return get_file_size(archive.file);
}

u64 serialize(Archive &archive, void *data, u64 size) {
    if (archive.mode == ARCHIVE_READ) {
        return read_file(archive.file, size, data);
    } else if (archive.mode == ARCHIVE_WRITE) {
        return write_file(archive.file, size, data);        
    }

    log(LOG_ERROR, "Unknown mode %u in archive 0x%X", archive.mode, &archive);
    return 0;
}

void reset(Archive &archive) {
    close_file(archive.file);
    archive.file = FILE_NONE;
}
