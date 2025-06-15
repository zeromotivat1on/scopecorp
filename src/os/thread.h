#pragma once

typedef u32 (* Thread_Entry)(void *);
typedef void *Thread;

extern const u32    WAIT_INFINITE;
extern const Thread THREAD_NONE;
extern const s32    THREAD_CREATE_IMMEDIATE;
extern const s32    THREAD_CREATE_SUSPENDED;

Thread os_thread_create(Thread_Entry entry, s32 create_type, void *user_data = null);
void   os_thread_resume(Thread handle);
void   os_thread_suspend(Thread handle);
void   os_thread_terminate(Thread handle);
bool   os_thread_is_active(Thread handle);
void   os_thread_sleep(u32 ms);
u64	   os_thread_get_current_id();
