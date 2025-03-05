#pragma once

#if DEBUG
#define assert(x) if (x) {} else { report_assert(#x, __FILE__, __LINE__); }
#elif RELEASE
#define assert(x)
#endif

void report_assert(const char* condition, const char* file, s32 line);
void debug_break();
