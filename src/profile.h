#pragma once

#include "os/time.h"

#ifdef TRACY_ENABLE
#include "tracy/tracy/Tracy.hpp"
#include "tracy/tracy/TracyC.h"
#define PROFILE_START(ctx, name) TracyCZoneN(ctx, name, true)
#define PROFILE_END(ctx)         TracyCZoneEnd(ctx)
#define PROFILE_SCOPE(name) ZoneScopedN(name)
#define PROFILE_FRAME(name) FrameMarkNamed(name)
#else
#define PROFILE_START(ctx, name) 
#define PROFILE_END(ctx)
#define PROFILE_SCOPE(name) Profile_Scope (profile_scope##__LINE__)(name, __FILE__, __LINE__)
#define PROFILE_FRAME(name)
#endif

#define SCOPE_TIMER_NAME(name) CONCAT(scope_timer_, name)
#define START_SCOPE_TIMER(name) const auto SCOPE_TIMER_NAME(name) = performance_counter()
#define CHECK_SCOPE_TIMER_S(name)  (performance_counter() - SCOPE_TIMER_NAME(name)) / (f32)performance_frequency_s()
#define CHECK_SCOPE_TIMER_MS(name) (performance_counter() - SCOPE_TIMER_NAME(name)) / (f32)performance_frequency_ms()

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
    
    f32 scope_time_update_interval = 0.5f;
    f32 scope_time_update_time = 0.0f;
    
    bool is_open = false;
};

inline Profiler profiler;

void init_profiler();
void draw_profiler();

void draw_dev_stats();
