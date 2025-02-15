#include "pch.h"
#include "my_time.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

s64 current_time_ms()
{
    return performance_counter() * 1000ull / performance_frequency();
}

s64 time_since_sys_boot_ms()
{
    return GetTickCount64();
}

s64 performance_counter()
{
    LARGE_INTEGER counter;
    const BOOL res = QueryPerformanceCounter(&counter);
    assert(res);
    return counter.QuadPart;
}

s64 performance_frequency()
{
    static u64 frequency64 = 0;

    if (frequency64 == 0)
    {
        LARGE_INTEGER frequency;
        const BOOL res = QueryPerformanceFrequency(&frequency);
        assert(res);
        frequency64 = frequency.QuadPart;
    }
    
    return frequency64;
}
