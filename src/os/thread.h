#pragma once

typedef u32(*Thread_Entry)(void *);
typedef void *Thread;

extern const u32 WAIT_INFINITE;
extern const s32 THREAD_CREATE_IMMEDIATE;
extern const s32 THREAD_CREATE_SUSPENDED;

Thread create_thread(Thread_Entry entry, void *userdata, s32 create_type);
void resume_thread(Thread handle);
void suspend_thread(Thread handle);
void terminate_thread(Thread handle);
bool thread_active(Thread handle);
void sleep_thread(u32 ms);
u64	current_thread_id();
