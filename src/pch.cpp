#include "pch.h"
#include "log.h"
#include "sid.h"
#include "hash.h"

#include "os/memory.h"

#include "editor/debug_console.h"

#include "game/game.h"
#include "game/world.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <malloc.h>
#include <string.h>

static void *vm_base     = null;
static void *allocl_base = null;
static void *allocf_base = null;
u64 allocl_size = 0;
u64 allocf_size = 0;

bool alloc_init() {
#if DEBUG
	void *vm_address = (void *)TB(2);
#else
	void *vm_address = null;
#endif
    
	vm_base = vm_reserve(vm_address, GB(1));
    if (!vm_base) {
        error("Failed to reserve virtual address space");
        return false;
    }

    u8 *commited = (u8 *)vm_commit(vm_base, MAX_ALLOCL_SIZE + MAX_ALLOCF_SIZE);
    if (!commited) {
        error("Failed to commit memory for Linear and Frame allocations");
        return false;
    }
    
    allocl_base = commited;
    allocf_base = commited + MAX_ALLOCL_SIZE;

    log("Allocated size: Linear %.2fmb | Frame %.2fmb", (f32)MAX_ALLOCL_SIZE / 1024 / 1024, (f32)MAX_ALLOCF_SIZE / 1024 / 1024);

    return true;
}

void alloc_shutdown() {
	vm_release(vm_base);
}

void *allocs(u64 size) {
    return alloca(size);
}

void *alloch(u64 size) {
    return malloc(size);
}

void *realloch(void *ptr, u64 size) {
    return realloc(ptr, size);
}

void freeh(void *ptr) {
    free(ptr);
}

void *allocl(u64 size) {
    Assert(allocl_size + size <= MAX_ALLOCL_SIZE);
    void *ptr = (u8 *)allocl_base + allocl_size;
    allocl_size += size;    
    return ptr;
}

void freel(u64 size) {
    Assert(allocl_size >= size);
    allocl_size -= size;
}

void *allocf(u64 size) {
	Assert(allocf_size + size <= MAX_ALLOCF_SIZE);
    void *ptr = (u8 *)allocf_base + allocf_size;
    allocf_size += size;    
    return ptr;
}

void freef() {
    allocf_size = 0;
}

void *alloclp(void **ptr, u64 size) {
    void *ret = *ptr;
    *ptr = (char *)*ptr + size;
    return ret;
}

void freelp(void **ptr, u64 size) {
    *ptr = (char *)*ptr - size;
}

void set_bytes(void *data, s32 value, u64 size) {
    memset(data, value, size);
}

void copy_bytes(void *dst, const void *src, u64 size) {
    memcpy(dst, src, size);
}

void move_bytes(void *dst, const void *src, u64 size) {
    memmove(dst, src, size);
}

#if DEBUG
void report_assert(const char *condition, const char *file, s32 line) {
	error("Assertion %s failed at %s:%d", condition, file, line);
	debug_break();
}

void debug_break() {
	__debugbreak();
}
#endif DEBUG

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

static u64 sid_table_hash(const u64 &a) {
    return a;
}

void init_sid_table() {
    sid_table = Sid_Table(MAX_SID_TABLE_SIZE);
    sid_table.hash_function = &sid_table_hash;
}

u64 cache_sid(const char *string) {
    const u64 hash = hash_fnv(string);
    if (sid_table.find(hash) == null) sid_table.add(hash, string);
    return hash;
}

u32 hash_pcg32(u32 input) {
    const u32 state = input * 747796405u + 2891336453u;
    const u32 word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u64 hash_pcg64(u64 input) {
    return hash_pcg32((u32)input) + hash_pcg32(input >> 32);
}

u64 hash_fnv(const char* str) {
    u64 hash = FNV_BASIS;
    for (const char* p = str; *p; ++p) {
        hash ^= (u64)(u8)(*p);
        hash *= FNV_PRIME;
    }

    return hash;
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
    case ENTITY_PLAYER:       return "ENTITY_PLAYER";
    case ENTITY_SKYBOX:       return "ENTITY_SKYBOX";
    case ENTITY_STATIC_MESH:  return "ENTITY_STATIC_MESH";
    case ENTITY_DIRECT_LIGHT: return "ENTITY_DIRECT_LIGHT";
    case ENTITY_POINT_LIGHT:  return "ENTITY_POINT_LIGHT";
    default:                  return "UNKNOWN";
    }
}
