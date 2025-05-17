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
#define PROFILE_SCOPE(name)
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

void draw_dev_stats();
