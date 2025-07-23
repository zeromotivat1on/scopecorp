#pragma once

#include "sparse_array.h"

struct Au_Sound;

struct Au_Table {
    static constexpr u32 MAX_SOUNDS = 32;
    
    Sparse_Array<Au_Sound> sounds;
};

inline Au_Table Au_table;

void au_create_table(Au_Table &t);
