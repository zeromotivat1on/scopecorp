#pragma once

#include "sparse_array.h"

#include "audio/sound.h"

inline constexpr s32 MAX_SOUNDS = 16;

struct Audio_Registry {
    Sparse_Array<Sound> sounds;    
};

inline Audio_Registry audio_registry;

void init_audio_context();
void init_audio_registry();
