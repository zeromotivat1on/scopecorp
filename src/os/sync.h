#pragma once

typedef void *Semaphore;
typedef void *Mutex;
typedef void *Critical_Section;

extern const Mutex     MUTEX_NONE;
extern const Semaphore SEMAPHORE_NONE;

extern const u32 WAIT_INFINITE;

Semaphore create_semaphore  (s32 init_count, s32 max_count);
bool      release_semaphore (Semaphore handle, s32 count, s32 *prev_count = null);
bool      wait_semaphore    (Semaphore handle, u32 ms);

Mutex create_mutex  (bool signaled);
bool  release_mutex (Mutex handle);
bool  wait_mutex    (Mutex handle, u32 ms);

// Create critical section with optional spin lock counter.
// If spin count <= 0, wait for critical section will always put calling thread to sleep.
// If thread tries to acquire locked critical section with spin lock, it will spin in a
// loop spin count times checking if lock is released. If lock is not released at the end
// of spin loop, thread is put to sleep.
Critical_Section create_cs    (u32 spin_count = 0);
void             enter_cs     (Critical_Section handle);
bool             try_enter_cs (Critical_Section handle);
void             leave_cs     (Critical_Section handle);
void             delete_cs    (Critical_Section handle);
