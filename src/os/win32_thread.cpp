#include "pch.h"
#include "thread.h"
#include <intrin.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const u32 WAIT_INFINITE = INFINITE;
const u32 INVALID_THREAD_RESULT = ((DWORD)-1);
const s32 THREAD_CREATE_IMMEDIATE = 0;
const s32 THREAD_CREATE_SUSPENDED = CREATE_SUSPENDED;
const u32 CRITICAL_SECTION_SIZE = sizeof(CRITICAL_SECTION);

static BOOL win32_wait_res_check(void* handle, DWORD res)
{
    switch(res)
    {
        case WAIT_OBJECT_0:
            return true;

        case WAIT_ABANDONED:
            log("Mutex (%p) was not released before owning thread termination\n", handle);
            return false;
            
        case WAIT_TIMEOUT:
            log("Wait time for object (%p) elapsed\n", handle);
            return false;
            
        case WAIT_FAILED:
            log("Failed to wait for object (%p) with error code (%u)\n", handle, GetLastError());
            return false;
                        
        default:
            log("Unknown wait result (%u) for object (%p)\n", res, handle);
            return false;
    }
}

u64 current_thread_id()
{
    return GetCurrentThreadId();
}

void sleep_thread(u32 ms)
{
    Sleep(ms);
}

bool thread_active(Thread handle)
{
    DWORD exit_code;
    GetExitCodeThread(handle, &exit_code);
    return exit_code == STILL_ACTIVE;
}

Thread create_thread(Thread_Entry entry, void* userdata, s32 create_type)
{
    return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)entry, userdata, create_type, NULL);
}

void resume_thread(Thread handle)
{
    const DWORD res = ResumeThread(handle);
    assert(res != INVALID_THREAD_RESULT);
}

void suspend_thread(Thread handle)
{
    const DWORD res = SuspendThread(handle);
    assert(res != INVALID_THREAD_RESULT);
}

void terminate_thread(Thread handle)
{
    DWORD exit_code;
    GetExitCodeThread(handle, &exit_code);
    const BOOL res = TerminateThread(handle, exit_code);
    assert(res);
}

Semaphore create_semaphore(s32 init_count, s32 max_count)
{
    return CreateSemaphore(NULL, (LONG)init_count, (LONG)max_count, NULL);
}

bool release_semaphore(Semaphore handle, s32 count, s32* prev_count)
{
    return ReleaseSemaphore(handle, count, (LPLONG)prev_count);
}

bool wait_semaphore(Semaphore handle, u32 ms)
{
    const DWORD res = WaitForSingleObjectEx(handle, ms, FALSE);
    return win32_wait_res_check(handle, res);
}

Mutex create_mutex(bool signaled)
{
    return CreateMutex(NULL, (LONG)signaled, NULL);
}

bool release_mutex(Mutex handle)
{
    return ReleaseMutex(handle);
}

bool wait_mutex(Mutex handle, u32 ms)
{
    const DWORD res = WaitForSingleObjectEx(handle, ms, FALSE);
    return win32_wait_res_check(handle, res);
}

void init_critical_section(Critical_Section handle, u32 spin_count)
{
    if (spin_count > 0)
        InitializeCriticalSectionAndSpinCount((LPCRITICAL_SECTION)handle, spin_count);
    else
        InitializeCriticalSection((LPCRITICAL_SECTION)handle);
}

void enter_critical_section(Critical_Section handle)
{
    EnterCriticalSection((LPCRITICAL_SECTION)handle);
}

bool try_enter_critical_section(Critical_Section handle)
{
    return TryEnterCriticalSection((LPCRITICAL_SECTION)handle);
}

void leave_critical_section(Critical_Section handle)
{
    LeaveCriticalSection((LPCRITICAL_SECTION)handle);
}

void delete_critical_section(Critical_Section handle)
{
    DeleteCriticalSection((LPCRITICAL_SECTION)handle);
}

void read_barrier()
{
    _ReadBarrier();
}

void write_barrier()
{
    _WriteBarrier();
}

void memory_barrier()
{
    _ReadWriteBarrier();
}

void read_fence()
{
    _mm_lfence();
}

void write_fence()
{
    _mm_sfence();
}

void memory_fence()
{
    _mm_mfence();
}

s32 atomic_swap(s32* dst, s32 val)
{
    return InterlockedExchange((LONG*)dst, val);
}

void* atomic_swap(void** dst, void* val)
{
    return InterlockedExchangePointer(dst, val);
}

s32 atomic_cmp_swap(s32* dst, s32 val, s32 cmp)
{
    return InterlockedCompareExchange((LONG*)dst, val, cmp);
}

void* atomic_cmp_swap(void** dst, void* val, void* cmp)
{
    return InterlockedCompareExchangePointer(dst, val, cmp);
}

s32 atomic_add(s32* dst, s32 val)
{
    return InterlockedAdd((LONG*)dst, val);
}

s32 atomic_increment(s32* dst)
{
    return InterlockedIncrement((LONG*)dst);
}

s32 atomic_decrement(s32* dst)
{
    return InterlockedDecrement((LONG*)dst);
}
