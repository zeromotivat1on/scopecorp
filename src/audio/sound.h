#pragma once

enum Sound_Flags : u32 {
    SOUND_FLAG_LOOP = 0x1,
};

struct Sound {
    u32 flags;
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
Sound create_sound(const char* path, u32 flags);
