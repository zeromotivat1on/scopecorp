#pragma once

typedef void *Semaphore;
typedef void *Mutex;
typedef void *Critical_Section;

extern const Mutex     INVALID_MUTEX;
extern const Semaphore INVALID_SEMAPHORE;

extern const u32 WAIT_INFINITE;
extern const u32 CRITICAL_SECTION_SIZE;

Semaphore create_semaphore(s32 init_count, s32 max_count);
bool release_semaphore(Semaphore handle, s32 count, s32 *prev_count);
bool wait_semaphore(Semaphore handle, u32 ms);

Mutex create_mutex(bool signaled);
bool release_mutex(Mutex handle);
bool wait_mutex(Mutex handle, u32 ms);

// Initialize critical section in preallocated memory with optional spin lock counter.
// If spin_count <= 0, wait for critical section will always put calling thread to sleep.
// If thread tries to acquire locked critical section with spin lock,
// thread spins in a loop spin_count times checking if lock is released.
// If lock is not released at the end of spin loop, thread is put to sleep.
void init_critical_section(Critical_Section handle, u32 spin_count);
void enter_critical_section(Critical_Section handle);
bool try_enter_critical_section(Critical_Section handle);
void leave_critical_section(Critical_Section handle);
void delete_critical_section(Critical_Section handle);
