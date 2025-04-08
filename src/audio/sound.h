#pragma once

inline constexpr s32 MAX_SOUND_SIZE = MB(64);

typedef u64 sid;
struct vec3;

enum Sound_Flags : u32 {
	SOUND_FLAG_LOOP = 0x1,
};

struct Sound_Memory {
    void *data = null;
    u32 size = 0;
    s32 sample_rate   = 0;
    s32 channel_count = 0;
	s32 bit_depth     = 0;
};

struct Sound {
	u32 flags;
	u32 buffer;
	u32 source;
    s32 sample_rate;
	s32 channel_count;
	s32 bit_depth;
	s32 audio_format;
};

struct Sound_Sid_List {
	sid world;
	sid player_steps;
	sid player_steps_cute;
};

inline Sound_Sid_List sound_sids;

void cache_sound_sids(Sound_Sid_List *list);

s32 create_sound(s32 sample_rate, s32 channel_count, s32 bit_depth, void *data, u32 size, u32 flags);

// Returned pointer is start of actual sound data.
void *extract_wav(void *data, s32 *channel_count, s32 *sample_rate, s32 *bits_per_sample, u32 *size);

Sound_Memory load_sound_memory(const char *path);
void free_sound_memory(Sound_Memory *memory);

void set_listener_pos(vec3 pos);
