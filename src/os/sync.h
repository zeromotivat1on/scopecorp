#pragma once

typedef void *Semaphore;
typedef void *Mutex;
typedef void *Critical_Section;

extern const Mutex     MUTEX_NONE;
extern const Semaphore SEMAPHORE_NONE;

extern const u32 WAIT_INFINITE;
extern const u32 CRITICAL_SECTION_SIZE;

Semaphore os_semaphore_create(s32 init_count, s32 max_count);
bool      os_semaphore_release(Semaphore handle, s32 count, s32 *prev_count = null);
bool      os_semaphore_wait(Semaphore handle, u32 ms);

Mutex os_mutex_create(bool signaled);
bool  os_mutex_release(Mutex handle);
bool  os_mutex_wait(Mutex handle, u32 ms);

// Initialize critical section in preallocated memory with optional spin lock counter.
// If spin_count <= 0, wait for critical section will always put calling thread to sleep.
// If thread tries to acquire locked critical section with spin lock,
// thread spins in a loop spin_count times checking if lock is released.
// If lock is not released at the end of spin loop, thread is put to sleep.
void os_cs_init(Critical_Section handle, u32 spin_count);
void os_cs_enter(Critical_Section handle);
bool os_cs_try_enter(Critical_Section handle);
void os_cs_leave(Critical_Section handle);
void os_cs_delete(Critical_Section handle);
