#include "pch.h"
#include "log.h"
#include "sid.h"
#include "hash.h"

#include "os/memory.h"

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

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define ALLOC_DEBUG 0

static void *vm_base     = null;
static void *allocp_base = null;
static void *alloct_base = null;
static void *allocf_base = null;
u64 allocp_size = 0;
u64 alloct_size = 0;
u64 allocf_size = 0;

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

bool alloc_init() {
#if DEVELOPER
	void *vm_address = (void *)TB((u64)2);
#else
	void *vm_address = null;
#endif
    
	vm_base = os_vm_reserve(vm_address, MB(128));
    if (!vm_base) {
        error("Failed to reserve virtual address space");
        return false;
    }

    constexpr u32 COMMIT_SIZE = MAX_ALLOCP_SIZE + MAX_ALLOCT_SIZE + MAX_ALLOCF_SIZE;
    u8 *commited = (u8 *)os_vm_commit(vm_base, COMMIT_SIZE);
    if (!commited) {
        error("Failed to commit memory for Linear and Frame allocations");
        return false;
    }
    
    allocp_base = commited;
    alloct_base = commited + MAX_ALLOCP_SIZE;
    allocf_base = commited + MAX_ALLOCP_SIZE + MAX_ALLOCT_SIZE;

    log("Allocation size: Persistent %.2fmb | Temp %.2fmb | Frame %.2fmb",
        (f32)MAX_ALLOCP_SIZE / 1024 / 1024,
        (f32)MAX_ALLOCT_SIZE / 1024 / 1024,
        (f32)MAX_ALLOCF_SIZE / 1024 / 1024);

    return true;
}

void alloc_shutdown() {
	os_vm_release(vm_base);
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

void *allocp(u64 size, Source_Location loc) {
#if ALLOC_DEBUG
    log("%s %s:%d %llu", __func__, loc.file, loc.line, size);
#endif
    
    Assert(allocp_size + size <= MAX_ALLOCP_SIZE);
    void *ptr = (u8 *)allocp_base + allocp_size;
    allocp_size += size;
    return ptr;
}

void freep(u64 size, Source_Location loc) {
#if ALLOC_DEBUG
    log("%s %s:%d %llu", __func__, loc.file, loc.line, size);
#endif
    
    Assert(allocp_size >= size);
    allocp_size -= size;
}

void *alloct(u64 size) {
    Assert(alloct_size + size <= MAX_ALLOCT_SIZE);
    void *ptr = (u8 *)alloct_base + alloct_size;
    alloct_size += size;
    return ptr;
}

void freet(u64 size) {
    Assert(alloct_size >= size);
    alloct_size -= size;
}

void *allocf(u64 size) {
	Assert(allocf_size + size <= MAX_ALLOCF_SIZE);
    void *ptr = (u8 *)allocf_base + allocf_size;
    allocf_size += size;    
    return ptr;
}

void freef(u64 size) {
    Assert(allocf_size >= size);
    allocf_size -= size;
}

void freef() {
    allocf_size = 0;
}

void mem_set(void *data, s32 value, u64 size) {
    memset(data, value, size);
}

void mem_copy(void *dst, const void *src, u64 size) {
    memcpy(dst, src, size);
}

void mem_move(void *dst, const void *src, u64 size) {
    memmove(dst, src, size);
}

void mem_swap(void *a, void *b, u64 size) {
    void *t = alloct(size);
    defer { freet(size); };
    
    mem_copy(t, a, size);
    mem_copy(a, b, size);
    mem_copy(b, t, size);
}

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

void sort(void *data, u32 count, u32 size, s32 (*compare)(const void*, const void*)) {
    qsort(data, count, size, compare);
}
