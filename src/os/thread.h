#pragma once

typedef u32 (* Thread_Entry)(void *);
typedef void *Thread;

extern const u32    WAIT_INFINITE;
extern const Thread THREAD_NONE;
extern const s32    THREAD_CREATE_IMMEDIATE;
extern const s32    THREAD_CREATE_SUSPENDED;

Thread os_create_thread(Thread_Entry entry, s32 create_type, void *user_data = null);
void   os_resume_thread(Thread handle);
void   os_suspend_thread(Thread handle);
void   os_terminate_thread(Thread handle);
bool   os_thread_active(Thread handle);
void   os_sleep_thread(u32 ms);
u64	   os_current_thread_id();
