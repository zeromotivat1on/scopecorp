#include "pch.h"
#include "log.h"
#include "str.h"
#include "hash.h"
#include "large.h"
#include "hash_table.h"

#include "os/system.h"
#include "os/memory.h"

#include "math/math_basic.h"

#include "editor/debug_console.h"

#include "game/game.h"
#include "game/world.h"

#include "stdlib.h"

// @Cleanyp: define stb related macros that allow to override usage of std.

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// @Todo: get rid of this.
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#if DEVELOPER
void report_assert(const char *condition, const char *msg, Source_Location loc) {
    if (msg) {
        error("Assertion (%s) failed at %s:%d with message: %s", condition, loc.file, loc.line, msg);
    } else {
        error("Assertion (%s) failed at %s:%d", condition, loc.file, loc.line);
    }
    
    debug_break();
}

void debug_break() {
    __debugbreak();
}
#endif

void asm_read_barrier() {
    _ReadBarrier();
}

void asm_write_barrier() {
    _WriteBarrier();
}

void asm_memory_barrier() {
    _ReadWriteBarrier();
}

void cpu_read_fence() {
    _mm_lfence();
}

void cpu_write_fence() {
    _mm_sfence();
}

void cpu_memory_fence() {
    _mm_mfence();
}

bool is_valid(Buffer b) {
    return b.data && b.size > 0;
}

bool reserve(Arena &a, u64 n, void *address) {
    Assert(!a.base);

    n = Alignup(n, OS_ALLOC_GRAN);
    a.base = (u8 *)os_reserve(address, n);
    
    if (!a.base) {
        error("Failed to reserve virtual memory of size %llu bytes for arena 0x%X", n, &a);
        return false;
    }
    
    a.reserved = n;
    return true;
}

bool commit(Arena &a, u64 n) {
    if (!a.base) return false;

    n = a.commited + n;
    n = Alignup(n, OS_PAGE_SIZE) - a.commited;

    if (a.commited + n > a.reserved) {
        error("Commit size of %llu bytes will result in overflow of reserved size %llu bytes in arena 0x%X", n, a.reserved, &a);
        return false;
    }
    
    void *p = a.base + a.commited;
    if (!os_commit(p, n)) {
        error("Failed to commit %llu bytes for arena 0x%X", n, &a);
        return false;
    }
    
    a.commited += n;
    return true;
}

void *push(Arena &a, u64 n, u64 align, bool zero) {
    Assert(Powerof2(align));
    
    if (!a.base) return null;
    
    const u64 curr_pos  = (u64)(a.base + a.used);
    const u64 align_pos = Alignup(curr_pos, align);
    const u64 padding   = align_pos - curr_pos;
    
    const u64 to_push = n + padding;
    if (a.used + to_push > a.reserved) {
        error("Push size of %llu bytes will result in overflow of reserved size %llu bytes in arena 0x%X", to_push, a.reserved, &a);
        return false;
    }
    
    if (a.used + to_push > a.commited) {
        // @Cleanup: maybe commit a bit more memory than we actually need?
        const u64 to_commit = a.used + to_push - a.commited;
        if (!commit(a, to_commit)) {
            return null;
        }
    }

    a.used += padding;
    
    u8 *data = a.base + a.used;
    a.used += n;
    
    if (zero) mem_set(data, 0, n);
    
    return data;
}

void pop(Arena &a, u64 n) {
    Assert(a.used >= n);
    a.used -= n;
}

void clear(Arena &a) {
    a.used = 0;
}

bool decommit(Arena &a, u64 n) {
    Assert(a.base);
    Assert(a.commited >= n);

    const u64 curr_pos    = (u64)(a.base + a.used);
    const u64 page_start  = Aligndown(curr_pos, OS_PAGE_SIZE);
    const u64 to_decommit = Alignup(n + (curr_pos - page_start), OS_PAGE_SIZE);
    
    void *p = (void *)page_start;
    if (!os_decommit(p, to_decommit)) {
        error("Failed to decommit %llu bytes from arena 0x%X", n, &a);
        return false;
    }

    a.commited -= to_decommit;

    return true;
}

bool release(Arena &a) {
    if (!a.base) return false;

    if (!os_release(a.base)) {
        error("Failed to release memory from arena 0x%X", &a);
        return false;
    }

    a.base = {};
    return true;
}

Scratch local_scratch() {
    static thread_local Arena arenas[4];
    static u32 index = 0;
    
    Arena &arena = arenas[index];
    index = (index + 1) % COUNT(arenas);
    
    if (!arena.base) {
        constexpr u64 to_reserve = MB(64);
        reserve(arena, to_reserve);
    }
    
    return { arena, arena.used };
}

void release(Scratch &s) {
    s.arena.used = s.pos;
}

void mem_set(void *p, s32 v, u64 n) {
    memset(p, v, n);
}

void mem_copy(void *dst, const void *src, u64 n) {
    memcpy(dst, src, n);
}

void mem_move(void *dst, const void *src, u64 n) {
    memmove(dst, src, n);
}

void mem_swap(void *a, void *b, u64 n) {
    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    void *t = push(scratch.arena, n);
    mem_copy(t, a, n);
    mem_copy(a, b, n);
    mem_copy(b, t, n);
}

// log

static void log_output_va(Log_Level log_level, const char *format, va_list args) {
    if (log_level == LOG_NONE) return;
    
	const char *prefixes[] = { "\x1b[37m", "\x1b[93m", "\x1b[91m" };

	static char buffer[4096];
	s32 count = stbsp_vsnprintf(buffer, sizeof(buffer), format, args);
	printf("%s%s", prefixes[log_level], buffer);

	// Restore default bg and fg colors.
	puts("\x1b[39;49m");

    buffer[count] = '\n';
    count += 1;
    add_to_debug_console_history(buffer, count);
}

void print(Log_Level level, const char *format, ...) {
    va_list args;
	va_start(args, format);
	log_output_va(level, format, args);
	va_end(args);
}

void log(const char *format, ...) {
	va_list args;
	va_start(args, format);
	log_output_va(LOG_LOG, format, args);
	va_end(args);
}

void warn(const char *format, ...) {
	va_list args;
	va_start(args, format);
	log_output_va(LOG_WARN, format, args);
	va_end(args);
}

void error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	log_output_va(LOG_ERROR, format, args);
	va_end(args);
}

// hash

u32 hash_pcg32(u32 input) {
    const u32 state = input * 747796405u + 2891336453u;
    const u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u64 hash_pcg64(u64 input) {
    return hash_pcg32((u32)input) + hash_pcg32(input >> 32);
}

u64 hash_fnv(String s) {
    u64 hash = FNV_BASIS;
    for (u64 i = 0; i < s.length; ++i) {
        hash ^= (u64)(u8)(s.value[i]);
        hash *= FNV_PRIME;
    }

    return hash;
}

// algo

void sort(void *data, u32 count, u32 size, s32 (*compare)(const void*, const void*)) {
    qsort(data, count, size, compare);
}

// string

String str_copy(Arena &a, const char *cs) {
    return str_copy(a, cs, strlen(cs));
}

String str_copy(Arena &a, const char *cs, u64 length) {
    // @Cleanup: not really sure how to handle this situation correctly, just cast for now.
    // I've supposed that C++ will understand that function takes const object and thus
    // allow to pass without cast to non-const pointer.
    return str_copy(a, String { (char *)cs, length });
}

String str_copy(Arena &a, String s) {
    String r;
    r.length = s.length;
    r.value  = arena_push_array(a, s.length, char);
    strncpy(r.value, s.value, s.length);
    return r;
}

static String str_format(Arena &a, const char *cs, const va_list &args) {
    // We do not assume really big strings for this function.
    constexpr u32 N = KB(16);
    
    String s;
    s.value  = arena_push_array(a, N, char);
	s.length = stbsp_vsnprintf(s.value, N, cs, args);

    arena_pop_array(a, N - s.length, char);
    
    return s;
}

String str_format(Arena &a, const char *cs, ...) {    
    va_list args;
	va_start(args, cs);
    defer {	va_end(args); };

    return str_format(a, cs, args);
}

String str_from(String s, s64 i) {    
    const u64 ai = Abs(i);
    if (ai >= s.length) return STRING_NONE;
    
    u64 pos = ai;
    if (i < 0) pos = s.length - ai;
    
    String r;
    r.value  = s.value  + pos;
    r.length = s.length - pos;
    
    return r;
}

String str_from_to(String s, s64 a, s64 b) {
    const u64 aa = Abs(a);
    const u64 ab = Abs(b);

    if (aa >= s.length) return STRING_NONE;
    if (ab >= s.length) return STRING_NONE;

    u64 apos = aa;
    if (a < 0) apos = s.length - aa;

    u64 bpos = ab;
    if (b < 0) bpos = s.length - ab;

    String r;
    r.value  = s.value + apos;
    r.length = bpos - apos + 1;
    
    return r;
}

String str_trim(String s) {
    if (!is_valid(s)) return STRING_NONE;
    
    String r = s;
    while (is_ascii_space(*(r.value)))                { r.value  += 1; }
    while (is_ascii_space(*(r.value + r.length - 1))) { r.length -= 1; }
    return r;
}

String str_slice(String s, char c, u32 bits) {
    char cs[2] = { c, '\0' };
    return str_slice(s, String { cs, 1 }, bits);
}

String str_slice(String s, String sub, u32 bits) {
    const s64 index = str_index(s, sub, bits);
    if (index < 0 || index > (s64)s.length) return STRING_NONE;

    String r;
    
    if (bits & S_LEFT_SLICE_BIT) {
        r.value  = s.value;
        r.length = index + 1;
    } else {
        r.value  = s.value  + index;
        r.length = s.length - index;
    }

    return r;
}

String str_token(String_Token_Iterator &it) {
    const auto length = it.s.length;
    const auto &s = it.s;

    u64 start = it.pos;
    while (start < length && str_index(it.delims, s.value[start]) != INDEX_NONE) {
        start++;
    }
    
    if (start >= length) {
        it.pos = length;
        return STRING_NONE;
    }
    
    u64 end = start;
    while (end < length && str_index(it.delims, s.value[end]) == INDEX_NONE) {
        end++;
    }

    it.pos = end;
    
    return String { s.value + start, end - start };
}

void str_c(String s, u64 length, char *cs) {
    Assert(s.length < length);
    strncpy(cs, s.value, s.length);
    cs[s.length] = '\0';
}

bool str_equal(String a, String b) {
    if (a.length != b.length) return false;
    return strncmp(a.value, b.value, a.length) == 0;
}

s64 str_index(String s, char c, u32 bits) {
    char cs[2] = { c, '\0' };
    return str_index(s, String { cs, 1 }, bits);
}

s64 str_index(String s, String sub, u32 bits) {
    if (sub.length == 0)       return 0;
    if (sub.length > s.length) return INDEX_NONE;

    s64 index = INDEX_NONE;

    s64 start = 0;
    if (bits & S_START_PLUS_ONE_BIT) {
        start += 1;
    }

    // @Todo: implement S_SEARCH_REVERSE_BIT.

    for (s64 i = start; i + sub.length <= s.length; ++i) {
        bool match = true;
        for (u64 j = 0; j < sub.length; ++j) {
            if (s.value[i + j] != sub.value[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            index = i;
            break;
        }
    }

    if (index == INDEX_NONE) {
        if (bits & S_LENGTH_ON_FAIL_BIT) {
            index = s.length;
        }
    } else {
        if (bits & S_INDEX_PLUS_ONE_BIT) {
            index += 1;
        }

        if (bits & S_INDEX_MINUS_ONE_BIT) {
            index -= 1;
        }
    }
    
    return index;
}

s32 str_to_s32(String s) {
    // We assume small strings to be passed here.
    constexpr u32 SIZE = 32;
    
    Assertm(s.length < SIZE, "Pass small strings that represent actual number");
    
    char buffer[SIZE];
    mem_copy(buffer, s.value, s.length);
    buffer[s.length] = '\0';
    
    return strtol(buffer, null, 0);
}

u32 str_to_u32(String s) {
    // We assume small strings to be passed here.
    constexpr u32 SIZE = 32;
    
    Assertm(s.length < SIZE, "Pass small strings that represent actual number");
    
    char buffer[SIZE];
    mem_copy(buffer, s.value, s.length);
    buffer[s.length] = '\0';
    
    return strtoul(buffer, null, 0);
}

f32 str_to_f32(String s) {
    // We assume small strings to be passed here.
    constexpr u32 SIZE = 32;
    
    Assertm(s.length < SIZE, "Pass small strings that represent actual number");
    
    char buffer[SIZE];
    mem_copy(buffer, s.value, s.length);
    buffer[s.length] = '\0';
    
    return (f32)strtod(buffer, null);
}


bool is_valid(String s) {
    return s.value && s.length > 0;
}

u64 str_size(const char *str) {
    return strlen(str);
}

void str_copy(char *dst, const char *src) {
    strcpy(dst, src);
}

void str_copy(char *dst, const char *src, u64 n) {
    strncpy(dst, src, n);
}

void str_glue(char *dst, const char *src) {
    strcat(dst, src);
}

void str_glue(char *dst, const char *src, u64 n) {
    strncat(dst, src, n);
}

bool str_cmp(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

bool str_cmp(const char *a, const char *b, u64 n) {
    return strncmp(a, b, n) == 0;
}

char *str_sub(char *str, const char *sub) {
    return strstr(str, sub);
}

char *str_char(char *str, s32 c) {
    return strchr(str, c);
}

char *str_char_from_end(char *str, s32 c) {
    return strrchr(str, c);
}

char *str_token(char *str, const char *delimiters) {
    return strtok(str, delimiters);
}

char *str_trim(char *str) {
    if (!str) return null;

    char *first = str;
    char *last  = str_last(str);

    while (is_space(*first)) {
        first += 1;
    }

    while (is_space(*last)) {
        last -= 1;
    }

    *(last + 1) = '\0';

    return first;
}

char *str_last(char *str) {
    const auto size = str_size(str);
    return str + size - 1;
}

const char *str_sub(const char *str, const char *sub) {
    return strstr(str, sub);
}

const char *str_char(const char *str, s32 c) {
    return strchr(str, c);
}

const char *str_char_from_end(const char *str, s32 c) {
    return strrchr(str, c);
}

bool is_space(s32 c) {
    return c == ASCII_SPACE
        || c == ASCII_TAB
        || c == ASCII_NEW_LINE
        || c == ASCII_FORM_FEED
        || c == ASCII_VERTICAL_TAB
        || c == ASCII_CARRIAGE_RETURN;
}

f32 str_to_f32(const char *str) {
    return (f32)atof(str);
}

s8 str_to_s8(const char *str) {
    return (s8)strtol(str, null, 0);
}

s16 str_to_s16(const char *str) {
    return (s16)strtol(str, null, 0);
}

s32 str_to_s32(const char *str) {
    return (s32)strtol(str, null, 0);
}

s64 str_to_s64(const char *str) {
    return strtol(str, null, 0);
}

u8 str_to_u8(const char *str) {
    return (u8)strtol(str, null, 0);
}

u16 str_to_u16(const char *str) {
    return (u16)strtol(str, null, 0);
}

u32 str_to_u32(const char *str) {
    return (u32)strtoul(str, null, 0);
}

u64 str_to_u64(const char *str) {
    return strtoul(str, null, 0);
}

const char *to_string(Game_Mode mode) {
	switch (mode) {
	case MODE_GAME:   return "GAME";
	case MODE_EDITOR: return "EDITOR";
	default:          return "UNKNOWN";
	}
}

const char *to_string(Camera_Behavior behavior) {
	switch (behavior) {
	case IGNORE_PLAYER:   return "IGNORE_PLAYER";
	case STICK_TO_PLAYER: return "STICK_TO_PLAYER";
	case FOLLOW_PLAYER:   return "FOLLOW_PLAYER";
	default:              return "UNKNOWN";
	}
}

const char *to_string(Player_Movement_Behavior behavior) {
	switch (behavior) {
	case MOVE_INDEPENDENT:        return "MOVE_INDEPENDENT";
	case MOVE_RELATIVE_TO_CAMERA: return "MOVE_RELATIVE_TO_CAMERA";
	default:                      return "UNKNOWN";
	}
}

const char *to_string(Entity_Type type) {
    switch (type) {
    case E_PLAYER:           return "E_PLAYER";
    case E_SKYBOX:           return "E_SKYBOX";
    case E_STATIC_MESH:      return "E_STATIC_MESH";
    case E_DIRECT_LIGHT:     return "E_DIRECT_LIGHT";
    case E_POINT_LIGHT:      return "E_POINT_LIGHT";
    case E_SOUND_EMITTER_2D: return "E_SOUND_EMITTER_2D";
    case E_SOUND_EMITTER_3D: return "E_SOUND_EMITTER_3D";
    case E_PORTAL:           return "E_PORTAL";
    default:                 return "UNKNOWN";
    }
}

// string builder

static void try_grow(Arena &a, String_Builder &sb, u64 to_grow) {
    if (sb.capacity - sb.buffer.length >= to_grow) return;

    const u64 new_capacity   = sb.capacity * 2 + to_grow;
    const u64 delta_capacity = new_capacity - sb.capacity;

    char *p = arena_push_array(a, delta_capacity, char);
    

    if (!sb.buffer.value) {
        sb.buffer.value = p;
    }
    
    sb.capacity = new_capacity;
}

static inline void append(Arena &a, String_Builder &sb, u64 count, const char *p) {
    try_grow(a, sb, count);
    strncpy(sb.buffer.value + sb.buffer.length, p, count);
    sb.buffer.length += count;
}

void str_build(Arena &a, String_Builder &sb, const char *cs) {
    const u64 length = strlen(cs);
    str_build(a, sb, cs, length);
}

void str_build(Arena &a, String_Builder &sb, const char *cs, u64 length) {
    str_build(a, sb, String { (char *)cs, length });
}

void str_build(Arena &a, String_Builder &sb, String s) {
    append(a, sb, s.length, s.value);
}

void str_build_format(Arena &a, String_Builder &sb, const char *cs, ...) {
    va_list args;
	va_start(args, cs);
    defer {	va_end(args); };

    String s = str_format(a, cs, args);
    append(a, sb, s.length, s.value);

    // @Hack: remove formatted string after append.
    arena_pop_array(a, s.length, char);
}

String str_build_finish(Arena &a, String_Builder &sb) {
    String s = sb.buffer;
    
    arena_pop_array(a, sb.capacity - sb.buffer.length, char);
    
    sb.buffer = STRING_NONE;
    sb.capacity = 0;
    
    return s;
}

// large

void set(u64 *buckets, u16 pos) {
    buckets[pos / 64] |= (1ull << (pos % 64));
}

void clear(u64 *buckets, u16 pos) {
    buckets[pos / 64] &= ~(1ull << (pos % 64));
}

void toggle(u64 *buckets, u16 pos) {
    buckets[pos / 64] ^= (1ull << (pos % 64));
}

bool check(const u64 *buckets, u16 pos) {
    return (buckets[pos / 64] & (1ull << (pos % 64))) != 0;
}

void set_whole(u64 *buckets, u8 count, u8 start) {
    mem_set(buckets + start, 0xFF, count * sizeof(u64));
}

void clear_whole(u64 *buckets, u8 count, u8 start) {
    mem_set(buckets + start, 0, count * sizeof(u64));
}

void And(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] & rbuckets[i];
    }
}

void Or(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] | rbuckets[i];
    }
}

void Xor(u64 *res_buckets, const u64 *lbuckets, const u64 *rbuckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = lbuckets[i] ^ rbuckets[i];
    }
}

void Not(u64 *res_buckets, const u64 *buckets, u8 count) {
    for (u8 i = 0; i < count; ++i) {
        res_buckets[i] = ~buckets[i];
    }
}

u256 operator&(const u256 &a, const u256 &b) {
    u256 res;
    And(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator|(const u256 &a, const u256 &b) {
    u256 res;
    Or(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator^(const u256 &a, const u256 &b) {
    u256 res;
    Xor(res.buckets, a.buckets, b.buckets, COUNT(res.buckets));
    return res;
}

u256 operator~(const u256 &a) {
    u256 res;
    Not(res.buckets, a.buckets, COUNT(res.buckets));
    return res;
}

bool operator<(const u256 &a, const u256 &b) {
    for (s32 i = 0; i < 4; ++i) {
        if (a.buckets[i] < b.buckets[i]) return true;
        if (a.buckets[i] > b.buckets[i]) return false;
    }
    return false;
}

bool operator>(const u256 &a, const u256 &b) {
    for (s32 i = 0; i < 4; ++i) {
        if (a.buckets[i] > b.buckets[i]) return true;
        if (a.buckets[i] < b.buckets[i]) return false;
    }
    return false;
}

bool operator<(const u512 &a, const u512 &b) {
    for (s32 i = 0; i < 8; ++i) {
        if (a.buckets[i] < b.buckets[i]) return true;
        if (a.buckets[i] > b.buckets[i]) return false;
    }
    return false;
}

bool operator>(const u512 &a, const u512 &b) {
    for (s32 i = 0; i < 8; ++i) {
        if (a.buckets[i] > b.buckets[i]) return true;
        if (a.buckets[i] < b.buckets[i]) return false;
    }
    return false;
}

// sid (string id)

static Table<sid, String> sid_table;
static char *sid_buffer = null; // storage for interned strings
static u64 sid_buffer_size = 0;

static u64 sid_table_hash(const sid &a) {
    return a;
}

void sid_init() {
    constexpr u16 MAX_TABLE_COUNT = 512;
    constexpr u32 MAX_BUFFER_SIZE = KB(16);

    // @Cleanup: use own arena?
    table_reserve(M_global, sid_table, MAX_TABLE_COUNT);
    table_custom_hash(sid_table, &sid_table_hash);

    // @Cleanup: use own arena?
    sid_buffer = (char *)arena_push_array(M_global, MAX_BUFFER_SIZE, char);
}

sid sid_intern(const char *cs) {
    return sid_intern(String { (char *)cs, strlen(cs) } );
}

sid sid_intern(String s) {
    const sid hash = (sid)hash_fnv(s);

    if (!table_find(sid_table, hash)) {
        Assert(s.length + sid_buffer_size <= table_bytes(sid_table));
        
        char *dst = sid_buffer + sid_buffer_size;
        mem_copy(dst, s.value, s.length);
        
        table_push(sid_table, hash, String { dst, s.length });
        
        sid_buffer_size += s.length;
    }
    
    return hash;
}

String sid_str(sid sid) {
    String *s = table_find(sid_table, sid);
    if (s) return *s;
    return STRING_NONE;
}
