#pragma once

#include "sync.h"
#include "thread.h"

struct Hot_Reload_Catalog {
    Catalog catalog;

    Array <String> directories;
    Array <String> paths_to_hot_reload;
    
    Semaphore semaphore = SEMAPHORE_NONE;
};

void   init_hot_reload          ();
Thread start_hot_reload_thread  ();
void   add_hot_reload_directory (String path);
void   update_hot_reload        ();
