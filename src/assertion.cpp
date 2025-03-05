#include "pch.h"
#include "assertion.h"
#include "log.h"

void report_assert(const char* condition, const char* file, s32 line) {
    error("Assertion %s failed at %s:%d", condition, file, line);
    debug_break();
}

void debug_break() {
    __debugbreak();
}
