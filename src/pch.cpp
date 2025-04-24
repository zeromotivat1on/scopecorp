#include "pch.h"
#include "log.h"
#include "sid.h"
#include "hash.h"

#include "game/game.h"
#include "game/world.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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
	const char *prefixes[] = { "\x1b[37m", "\x1b[93m", "\x1b[91m" };

	char buffer[1024] = { 0 };
	stbsp_vsnprintf(buffer, sizeof(buffer), format, args);
	printf("%s%s", prefixes[log_level], buffer);

	// Restore default bg and fg colors.
	puts("\x1b[39;49m");
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
    case ENTITY_PLAYER:      return "ENTITY_PLAYER";
    case ENTITY_SKYBOX:      return "ENTITY_SKYBOX";
    case ENTITY_STATIC_MESH: return "ENTITY_STATIC_MESH";
    default:                 return "UNKNOWN";
    }
}
