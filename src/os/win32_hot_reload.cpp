#include "pch.h"
#include "editor/hot_reload.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>

static constexpr s32 HOT_RELOAD_BUFFER_SIZE = 4096;

static DWORD hot_reload_proc(LPVOID param)
{
    auto data = (Hot_Reload_Directory*)param;
    auto dir = data->path;
    auto callback = data->callback;

    auto list = (Hot_Reload_List*)param;

    OVERLAPPED overlaps[MAX_HOT_RELOAD_LIST_SIZE] = {0};
    HANDLE handles[MAX_HOT_RELOAD_LIST_SIZE] = {0};
    HANDLE events[MAX_HOT_RELOAD_LIST_SIZE] = {0};
    char buffers[MAX_HOT_RELOAD_LIST_SIZE][HOT_RELOAD_BUFFER_SIZE] = {0};
    char* file_paths = (char*)LocalAlloc(LPTR, MAX_HOT_RELOAD_LIST_SIZE * MAX_PATH_SIZE);

    while (list->watch_count > 0)
    {
        for (s32 i = 0; i < list->watch_count; ++i)
        {
            auto& handle  = handles[i];
            auto& overlap = overlaps[i];
            auto& event   = events[i];
            auto& buffer  = buffers[i];

            if (!handle)
                handle = CreateFile(dir, GENERIC_READ|FILE_LIST_DIRECTORY, 
                                    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                    NULL, OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
            if (!event)
            {
                event = CreateEvent(NULL, TRUE, FALSE, NULL);
                overlap.hEvent = event;
            }


            DWORD bytes;
            BOOL result = ReadDirectoryChangesW(handle, buffer, HOT_RELOAD_BUFFER_SIZE, TRUE,
                                                FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes, &overlap, NULL);
            if (!result)
            {
                //log("Failed to read directory changes for %s", list->dirs[i].path);
                continue;
            }
        }

        const DWORD wait_res = WaitForMultipleObjects(list->watch_count, events, FALSE, INFINITE);

        if (wait_res == WAIT_FAILED)
        {
            const DWORD error = GetLastError();
            //log("Wait for directories hot reload failed with error %u", error);
            continue;
        }
        
        for (s32 i = 0; i < list->watch_count; ++i)
        {
            if (wait_res == WAIT_OBJECT_0 + i)
            {
                auto& dir    = list->dirs[i];
                auto& buffer = buffers[i];
                
                FILE_NOTIFY_INFORMATION* notify = null;
                s32 offset = 0;
                
                do
                {
                    notify = (FILE_NOTIFY_INFORMATION*)(buffer + offset);
                    auto full_path = file_paths + (i * MAX_PATH_SIZE);
                    strcpy_s(full_path, MAX_PATH_SIZE, dir.path);

                    char relative_path[128] = {0};
                    const s32 length = WideCharToMultiByte(CP_ACP, 0, notify->FileName, notify->FileNameLength / 2, relative_path, sizeof(relative_path), NULL, NULL);

                    assert(length < MAX_PATH_SIZE);
                    strcat_s(full_path, MAX_PATH_SIZE, relative_path);
                    
                    if (notify->Action == FILE_ACTION_MODIFIED)
                    {
                        if (dir.callback) dir.callback(full_path);
                    }
                    
                    offset += notify->NextEntryOffset;
                } while (notify->NextEntryOffset);                
            }
        }
    }

    LocalFree((HLOCAL)file_paths);

    for (s32 i = 0; i < MAX_HOT_RELOAD_LIST_SIZE; ++i)
    {
        auto handle = handles + i;
        auto event  = events + i;

        if (handle) CloseHandle(handle);
        if (event)  CloseHandle(event);
    }
    
    return 0;
}

void register_hot_reload_dir(Hot_Reload_List* list, const char* path, Hot_Reload_Callback callback)
{
    assert(list->watch_count < MAX_HOT_RELOAD_LIST_SIZE);
    
    auto dir = list->dirs + list->watch_count;
    dir->path = path;
    dir->callback = callback;
    
    list->watch_count++;

    log("Registered hot reload directory %s", path);
}

void start_hot_reload_thread(Hot_Reload_List* list)
{
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hot_reload_proc, list, 0, NULL);
    log("Started hot reload thread");
}

void stop_hot_reload_thread(Hot_Reload_List* list)
{
    list->watch_count = 0;
}
