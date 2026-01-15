#pragma once

#include "hash_table.h"
#include "catalog.h"

enum Sound_Bits : u32 {
    SOUND_LOOP_BIT = 0x1,
};

struct Sound {
    Atom   name;
	Handle buffer;
	Handle source;
    u32    bits;
    u16    channel_count;
    u16    bit_rate;
    u32    sample_rate;
	u32    audio_format;    
    u32    data_size;
};

struct Audio_Player {
    Handle              audio_device;
    Handle              audio_context;
    Vector3             listener_position;
    Table <Atom, Sound> sound_table;
    Array <Atom>        playing_sounds;
};

Audio_Player *get_audio_player            ();
bool          init_audio_player           ();
void          update_audio                ();
Vector3       get_audio_listener_position ();
void          set_audio_listener_position (Vector3 position);
Sound        *new_sound                   (Atom name, Buffer contents);
Sound        *get_sound                   (Atom name);
void          play_sound                  (const Sound *sound);
void          play_sound                  (const Sound *sound, Vector3 position);
void          stop_sound                  (const Sound *sound);
void          set_looping                 (Sound *sound, bool loop);
void          play_sound                  (Atom name);
void          play_sound                  (Atom name, Vector3 position);
void          stop_sound                  (Atom name);
void          set_sound_looping           (Atom name, bool loop);
