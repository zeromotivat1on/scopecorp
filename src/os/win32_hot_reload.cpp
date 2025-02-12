#include "pch.h"
#include "editor/hot_reload.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>

static DWORD hot_reload_proc(LPVOID param)
{
    auto data = (Hot_Reload_Directory*)param;
    auto dir = data->path;
    auto callback = data->callback;

    // @Cleanup: store in thread local storage?
    static char filename[256];
    
    HANDLE hdir = CreateFile(dir, GENERIC_READ|FILE_LIST_DIRECTORY, 
                             FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL,
                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
                             NULL);
    if (hdir == INVALID_HANDLE_VALUE) return 1;
    
    char buffer[4096];
    
    OVERLAPPED overlap = {0};
    overlap.OffsetHigh = 0;
    overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BOOL result = TRUE;
    while (result)
    {
        DWORD bytes = 0;
        result = ReadDirectoryChangesW(hdir,
                                       &buffer, sizeof(buffer), TRUE,
                                       FILE_NOTIFY_CHANGE_LAST_WRITE,
                                       &bytes, &overlap, NULL);
        
        WaitForSingleObject(overlap.hEvent, INFINITE);

        FILE_NOTIFY_INFORMATION* notify = null;
        s32 offset = 0;
        
        do
        {
            notify = (FILE_NOTIFY_INFORMATION*)(buffer + offset);

            const s32 length = WideCharToMultiByte(CP_ACP, 0, notify->FileName, notify->FileNameLength / 2, filename, sizeof(filename), NULL, NULL);
            filename[length] = '\0';
            
            if (notify->Action == FILE_ACTION_MODIFIED) callback(filename);

            offset += notify->NextEntryOffset;
        } while (notify->NextEntryOffset);
    }

    CloseHandle(hdir);
    return 0;
}

void register_hot_reload_dir(Hot_Reload_List* list, const char* path, Hot_Reload_Callback callback)
{
    assert(list->watch_count <= MAX_HOT_RELOAD_LIST_SIZE);
    
    auto dir = list->dirs + list->watch_count;
    dir->path = path;
    dir->callback = callback;
    
    list->watch_count++;
    
    // @Cleanup: destroy thread when done.
    // @Cleanup: or check directory synchronously?
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)hot_reload_proc, dir, 0, NULL);
}
