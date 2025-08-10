#pragma once

#include "log.h"
#include "os/time.h"

enum Input_Key : u8;

extern Input_Key KEY_SWITCH_RUNTIME_PROFILER;
extern Input_Key KEY_SWITCH_MEMORY_PROFILER;

#define SCOPE_TIMER_NAME(name) CONCAT(scope_timer_, name)
#define START_SCOPE_TIMER(name) const auto SCOPE_TIMER_NAME(name) = os_perf_counter()
#define CHECK_SCOPE_TIMER_S(name)  (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz_s()
#define CHECK_SCOPE_TIMER_MS(name) (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz_ms()

#define SCOPE_TIMER(name) Scope_Timer (scope_timer##__LINE__)(name)
struct Scope_Timer {
    const char *info = null;
	u64 start = 0;
    
	Scope_Timer(const char *info) : info(info), start(os_perf_counter()) {}
	~Scope_Timer() { log("%s %.2fms", info, (f32)(os_perf_counter() - start) / os_perf_hz_ms()); }
};

struct Memory_Profiler {
    bool is_open = false;
};

struct Window_Event;

inline Memory_Profiler  Memory_profiler;

void init_memory_profiler();
void open_memory_profiler();
void close_memory_profiler();
void draw_memory_profiler();
void on_input_memory_profiler(const Window_Event &event);

void draw_dev_stats();
