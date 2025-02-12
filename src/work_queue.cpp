#include "pch.h"
#include "work_queue.h"

void Work_Queue::init(Semaphore semaphore)
{
    *this = {0};
    this->semaphore = semaphore;
}

bool Work_Queue::active()
{
    return processed_entry_count != added_entry_count;
}

void Work_Queue::add(void* data, Callback callback)
{
    const u32 curr_entry_to_add = entry_to_add;
    const u32 next_entry_to_add = (curr_entry_to_add + 1) % ARRAY_COUNT(entries);

    ASSERT(next_entry_to_add != entry_to_process); // overflow

    const u32 idx = atomic_cmp_swap((volatile s32*)&entry_to_add, next_entry_to_add, curr_entry_to_add);
    if (idx == curr_entry_to_add)
    {
        Entry& entry = entries[curr_entry_to_add];
        entry.callback = callback;
        entry.data = data;

        atomic_increment((volatile s32*)&added_entry_count);
        release_semaphore(semaphore, 1, nullptr);
    }
}

bool Work_Queue::process()
{
    const u32 curr_entry_to_process = entry_to_process;
    if (curr_entry_to_process == entry_to_add)
    {
        return false;
    }

    const u32 next_entry_to_process = (curr_entry_to_process + 1) % ARRAY_COUNT(entries);
    const u32 idx = atomic_cmp_swap((volatile s32*)&entry_to_process, next_entry_to_process, curr_entry_to_process);
    if (idx == curr_entry_to_process)
    {
        Entry& entry = entries[curr_entry_to_process];
        entry.callback(this, entry.data);
        atomic_increment((volatile s32*)&processed_entry_count);
    }

    return true;
}

void Work_Queue::wait(u32 ms)
{
    wait_semaphore(semaphore, ms);
}
