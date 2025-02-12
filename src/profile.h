#pragma once

#if RELEASE
#define SCOPE_TIMER(Name)	
#else
#define SCOPE_TIMER(Name) Scope_Timer (scope_timer##__LINE__)(Name)

struct Scope_Timer
{
    Scope_Timer(const char* name) : name(name) { start = glfwGetTime(); }
    ~Scope_Timer() { printf("Timer (%s) took %.2fms\n", name, (glfwGetTime() - start) * 1000.0); }

	const char*	name;
	f64 start;
};
#endif
