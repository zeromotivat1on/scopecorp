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

#include "log.h"
#include "os/time.h"

#define SCOPE_TIMER(name) Scope_Timer (scope_timer##__LINE__)(name)

struct Scope_Timer {
	Scope_Timer(const char *info)
		: info(info), start(performance_counter()) {}

	~Scope_Timer() {
		const f32 seconds = (performance_counter() - start) / (f32)performance_frequency();
		log("%s %.2fms", info, seconds * 1000.0f);
	}

	const char *info;
	s64 start;
};
#endif

struct Font_Atlas;
struct World;

void draw_dev_stats(const Font_Atlas *atlas, const World *world);
