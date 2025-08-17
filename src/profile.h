#pragma once

#include "log.h"
#include "os/time.h"

enum Input_Key : u8;

extern Input_Key KEY_SWITCH_TELEMETRY;
extern Input_Key KEY_SWITCH_MEMORY_PROFILER;

#define SCOPE_TIMER_NAME(name) CONCAT(_st_, name)
#define START_SCOPE_TIMER(name)    const auto SCOPE_TIMER_NAME(name) = os_perf_counter()
#define CHECK_SCOPE_TIMER(name)    (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz()
#define CHECK_SCOPE_TIMER_MS(name) (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz_ms()

#define SCOPE_TIMER(info) const Scope_Timer SCOPE_TIMER_NAME(__LINE__)(info)
struct Scope_Timer {
    const char *info = null;
	u64 start = 0;
    
	Scope_Timer(const char *info) : info(info), start(os_perf_counter()) {}
	~Scope_Timer() { log("%s %.2fms", info, (f32)(os_perf_counter() - start) / os_perf_hz_ms()); }
};

enum Mprof_Sort_Type : u8 {
    MPROF_SORT_NONE,
    MPROF_SORT_NAME,
    MPROF_SORT_USED,
    MPROF_SORT_CAP,
    MPROF_SORT_PERC,
};

enum Mprof_Precision_Type : u8 {
    MPROF_BYTES,
    MPROF_KILO_BYTES,
    MPROF_MEGA_BYTES,
    MPROF_GIGA_BYTES,
    MPROF_TERA_BYTES,
};

struct Mprof_Context {
    u8 sort_type = 0;
    u8 precision_type = MPROF_MEGA_BYTES;
    bool is_open = false;
};

struct Window_Event;

void mprof_init();
void mprof_open();
void mprof_close();
void mprof_draw();
void mprof_on_input(const Window_Event &event);

void draw_dev_stats();
