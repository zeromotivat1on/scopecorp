#pragma once

#include "hash_table.h"
#include "catalog.h"

union Audio_Handle {
    u32 _u32;
    u64 _u64;
    void *_p;
};

inline constexpr auto AUDIO_HANDLE_NONE = Audio_Handle {0};

struct Audio_Player {
    Audio_Handle audio_device  = AUDIO_HANDLE_NONE;
    Audio_Handle audio_context = AUDIO_HANDLE_NONE;
    
    Vector3 listener_position = Vector3_zero;

    Catalog sound_catalog;
    
    Table <String, struct Sound> sound_table;
};

Audio_Player *get_audio_player   ();
Vector3 get_audio_listener_position ();
void set_audio_listener_position (Vector3 position);

bool init_audio_player ();
void update_audio      ();

enum Sound_Bits : u32 {
    SOUND_LOOP_BIT = 0x1,
};

struct Sound {
    String path;
    String name;
    
    u32 bits = 0;
    
	Audio_Handle buffer = 0;
	Audio_Handle source = 0;
    
    u16 channel_count = 0;
    u16 bit_rate = 0;
    u32 sample_rate = 0;
	u32 audio_format = 0;
    
    u32 data_size = 0;
};

Sound *new_sound   (String path);
Sound *new_sound   (String path, Buffer contents);
Sound *get_sound   (String name);
void   play_sound  (const Sound *sound);
void   play_sound  (const Sound *sound, Vector3 position);
void   stop_sound  (const Sound *sound);
void   set_looping (Sound *sound, bool loop);
void   play_sound  (String name);
void   play_sound  (String name, Vector3 position);
void   stop_sound  (String name);
void   set_sound_looping (String name, bool loop);
