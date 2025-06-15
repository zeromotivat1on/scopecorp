#pragma once

#include "os/time.h"

extern s16 KEY_SWITCH_PROFILER;

#define PROFILE_SCOPE(name) Profile_Scope (profile_scope##__LINE__)(name, __FILE__, __LINE__)

#define SCOPE_TIMER_NAME(name) CONCAT(scope_timer_, name)
#define START_SCOPE_TIMER(name) const auto SCOPE_TIMER_NAME(name) = os_perf_counter()
#define CHECK_SCOPE_TIMER_S(name)  (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_frequency_s()
#define CHECK_SCOPE_TIMER_MS(name) (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_frequency_ms()

#define SCOPE_TIMER(name) Scope_Timer (scope_timer##__LINE__)(name)
struct Scope_Timer {
    const char *info;
	s64 start;
    
	Scope_Timer(const char *info);
	~Scope_Timer();
};

struct Profile_Scope {
    s64 start = 0;
    s64 end = 0;
    s64 diff = 0;
    const char *name = null;
    const char *filepath = null;
    u32 line = 0;
    
    Profile_Scope(const char *scope_name, const char *scope_filepath, u32 scope_line);
    ~Profile_Scope();
};

inline constexpr u32 MAX_PROFILER_SCOPES = 1024;

struct Profiler {
    Profile_Scope *scopes = null;
    f32 *scope_times = null;
    u32 scope_count = 0;
    
    f32 scope_time_update_interval = 0.1f;
    f32 scope_time_update_time = 0.0f;
    
    bool is_open = false;
};

struct Window_Event;

inline Profiler profiler;

void init_profiler();
void open_profiler();
void close_profiler();
void draw_profiler();
void on_input_profiler(Window_Event *event);

void draw_dev_stats();
