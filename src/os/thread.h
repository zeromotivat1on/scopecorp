#pragma once

extern const u32 WAIT_INFINITE;
extern const u32 INVALID_THREAD_RESULT;
extern const s32 THREAD_CREATE_IMMEDIATE;
extern const s32 THREAD_CREATE_SUSPENDED;
extern const u32 CRITICAL_SECTION_SIZE;

typedef u32 (*Thread_Entry)(void*);

typedef void* Thread;
typedef void* Semaphore;
typedef void* Mutex;
typedef void* Critical_Section;

Thread create_thread(Thread_Entry entry, void* userdata, s32 create_type);
void resume_thread(Thread handle);
void suspend_thread(Thread handle);
void terminate_thread(Thread handle);
bool thread_active(Thread handle);
void sleep_thread(u32 ms);
u64	current_thread_id();

Semaphore create_semaphore(s32 init_count, s32 max_count);
bool release_semaphore(Semaphore handle, s32 count, s32* prev_count);
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

// Hint compiler to not reorder memory operations, happened before barrier.
// Basically disable memory read/write concerned optimizations.
void read_barrier();
void write_barrier();
void memory_barrier();

// Ensure the order of read/write CPU instructions.
void read_fence();
void write_fence();
void memory_fence();

// If dst is equal to cmp, set dst to val, otherwise do nothing, return dst before op.
void* atomic_cmp_swap(void** dst, void* val, void* cmp);
void* atomic_swap(void** dst, void* val);
s32	atomic_cmp_swap(s32* dst, s32 val, s32 cmp);
s32	atomic_swap(s32* dst, s32 val); // swap dst and val and return dst before op
s32	atomic_add(s32* dst, s32 val); // add val to dst and return dst before op
s32	atomic_increment(s32* dst);
s32	atomic_decrement(s32* dst);
