#pragma once

typedef void *Thread;

extern const u32    WAIT_INFINITE;
extern const Thread THREAD_NONE;

enum Thread_Bits : u32 {
    THREAD_SUSPENDED_BIT,
};

Thread create_thread    (u32 (*entry)(void *), u32 bits, void *user_data = null);
bool   resume_thread    (Thread handle);
bool   suspend_thread   (Thread handle);
bool   terminate_thread (Thread handle);
bool   is_active_thread (Thread handle);

void sleep                 (u32 ms);
u64	 get_current_thread_id ();
