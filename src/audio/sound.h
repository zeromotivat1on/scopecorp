#pragma once

#include "asset.h"

inline constexpr s32 MAX_SOUND_SIZE = MB(32);

typedef u64 sid;
struct vec3;

enum Sound_Flags : u32 {
	SOUND_FLAG_LOOP = 0x1,
};

struct Sound : Asset {
    Sound() { asset_type = ASSET_SOUND; }
    
	u32 flags = 0;
	u32 buffer = 0;
	u32 source = 0;
    s32 channel_count = 0;
    s32 sample_rate = 0;
	s32 bit_rate = 0;
	s32 audio_format = 0;
};

void au_init_context();
void au_play_sound(sid sid);
void au_play_sound_or_continue(sid sid); // do not play from start if already playing
void au_play_sound_or_continue(sid sid, vec3 location);
void au_stop_sound(sid sid);

void init_sound_asset(Sound *sound, void *data);

// Returns start of actual sound data.
void *extract_wav(void *data, s32 *channel_count, s32 *sample_rate, s32 *bits_per_sample, u32 *size);

void au_set_listener_pos(vec3 pos);
vec3 au_get_listener_pos();
