#pragma once

struct vec3;

#define AU_LOOP_BIT 0x1

struct Au_Sound {
    static constexpr u32 MAX_FILE_SIZE = MB(32);
    
    u32 bits = 0;
	u32 buffer = 0;
	u32 source = 0;
    u16 channel_count = 0;
    u16 bit_rate = 0;
    u32 sample_rate = 0;
	u32 audio_format = 0;
    u32 data_size = 0;

    struct Meta {
        u16 channel_count = 0;
        u16 bit_rate = 0;
        u32 sample_rate = 0;
    };
};

u16  au_create_sound(u16 ch_count, u16 bit_rate, u16 sample_rate, u32 data_size, void *data, u32 bits);
void au_play_sound(u16 sound);
void au_play_sound_or_continue(u16 sound);
void au_play_sound_or_continue(u16 sound, vec3 location);
void au_stop_sound(u16 sound);
