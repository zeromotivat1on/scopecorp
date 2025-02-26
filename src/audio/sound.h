#pragma once

struct Sound {
    u32 buffer;
    u32 source;
    s32 channel_count;
    s32 sample_rate;
    s32 bit_depth;
    s32 audio_format;
    const char* path;
};

struct Sound_List {
    Sound world;
    Sound player_steps;
    Sound player_steps_cute;
};

inline Sound_List sounds;

void load_game_sounds(Sound_List* list);
Sound create_sound(const char* path, bool loop);

// Extract wave header data and return pointer to start of actual sound data.
void* extract_wav(void* data, s32* channel_count, s32* sample_rate, s32* bits_per_sample, s32* size);
