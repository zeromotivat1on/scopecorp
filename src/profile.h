#pragma once

#ifdef TRACY_ENABLE
#include "tracy/tracy/Tracy.hpp"

#define PROFILE_SCOPE(name) ZoneScopedN(name)
#define PROFILE_FRAME(name) FrameMarkNamed(name)
#else
#define PROFILE_SCOPE(name)
#define PROFILE_FRAME(name)
#endif

#if RELEASE
#define SCOPE_TIMER(name)
#else

#define SCOPE_TIMER(name) Scope_Timer (scope_timer##__LINE__)(name)

struct Scope_Timer {
    const char *info;
	s64 start;
    
	Scope_Timer(const char *info);
	~Scope_Timer();
};
#endif

struct Font_Atlas;
struct World;

void draw_dev_stats(const Font_Atlas *atlas, const World *world);
