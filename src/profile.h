#pragma once

#include "os/time.h"

enum Input_Key : u8;

extern Input_Key KEY_SWITCH_RUNTIME_PROFILER;
extern Input_Key KEY_SWITCH_MEMORY_PROFILER;

#define PROFILE_SCOPE(name) Profile_Scope (profile_scope##__LINE__)(name, __FILE__, __LINE__)

#define SCOPE_TIMER_NAME(name) CONCAT(scope_timer_, name)
#define START_SCOPE_TIMER(name) const auto SCOPE_TIMER_NAME(name) = os_perf_counter()
#define CHECK_SCOPE_TIMER_S(name)  (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz_s()
#define CHECK_SCOPE_TIMER_MS(name) (os_perf_counter() - SCOPE_TIMER_NAME(name)) / (f32)os_perf_hz_ms()

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
    const char *file_path = null;
    u32 line = 0;
    
    Profile_Scope(const char *scope_name, const char *scope_file_path, u32 scope_line);
    ~Profile_Scope();
};

struct Runtime_Profiler {
    static constexpr u32 MAX_SCOPES = 1024;
    static constexpr u32 MAX_NAME_LENGTH = 32;
    
    Profile_Scope *scopes = null;
    f32 *scope_times = null;
    u32 scope_count = 0;
    
    f32 scope_time_update_interval = 0.2f;
    f32 scope_time_update_time = 0.0f;
    
    bool is_open = false;
};

struct Memory_Profiler {
    bool is_open = false;
};

struct Window_Event;

inline Runtime_Profiler Runtime_profiler;
inline Memory_Profiler  Memory_profiler;

void init_runtime_profiler();
void open_runtime_profiler();
void close_runtime_profiler();
void draw_runtime_profiler();
void on_input_runtime_profiler(const Window_Event &event);

void init_memory_profiler();
void open_memory_profiler();
void close_memory_profiler();
void draw_memory_profiler();
void on_input_memory_profiler(const Window_Event &event);

void draw_dev_stats();
