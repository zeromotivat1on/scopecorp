#pragma once

#if RELEASE
#define SCOPE_TIMER(Name)	
#else

#include "my_time.h"
#define SCOPE_TIMER(Name) Scope_Timer (scope_timer##__LINE__)(Name)

struct Scope_Timer
{
    Scope_Timer(const char* info) : info(info)
    {
        start = performance_counter();
    }
    
    ~Scope_Timer()
    {
        const f32 seconds = (performance_counter() - start) / (f32)performance_frequency();
        log("%s %.2fms", info, seconds * 1000.0f);
    }

	const char*	info;
	s64 start;
};
#endif
