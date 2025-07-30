#include "pch.h"
#include "audio/al.h"
#include "audio/alc.h"
#include "audio/au_sound.h"
#include "audio/au_table.h"

#include "math/vector.h"

#include "log.h"
#include "asset.h"

#define al_check_error() {                          \
        ALenum al_error = alGetError();             \
        while (al_error != AL_NO_ERROR) {           \
            error("OpenAL error 0x%X at %s:%d",     \
                  al_error, __FILE__, __LINE__);    \
            al_error = alGetError();                \
        }                                           \
    }

void au_init_context() {
	ALCdevice *audio_device = alcOpenDevice(null);
	if (!audio_device) {
		error("Failed to open default audio device");
	}

	ALCcontext *audio_context = alcCreateContext(audio_device, null);
	if (!audio_context) {
		error("Failed to create alc context");
	}

	if (!alcMakeContextCurrent(audio_context)) {
		error("Failed to make alc context current");
	}
}

static s32 to_al_audio_format(s32 channel_count, s32 bit_rate) {
	if (channel_count == 1 && bit_rate == 8)       return AL_FORMAT_MONO8;
	else if (channel_count == 1 && bit_rate == 16) return AL_FORMAT_MONO16;
	else if (channel_count == 2 && bit_rate == 8)  return AL_FORMAT_STEREO8;
	else if (channel_count == 2 && bit_rate == 16) return AL_FORMAT_STEREO16;
	else {
		error("Unknown audio format with channel count %d and bit rate %d", channel_count, bit_rate);
		return 0;
	}
}

u16 au_create_sound(u16 ch_count, u16 bit_rate, u16 sample_rate, u32 data_size, void *data, u32 bits) {
    Au_Sound sn;
    sn.bits = bits;
    sn.channel_count = ch_count;
    sn.bit_rate = sn.bit_rate;
    sn.sample_rate = sn.sample_rate;
    sn.data_size = data_size;
    sn.audio_format = to_al_audio_format(ch_count, bit_rate);
    
    // @Cleanup: loading big sounds like this is bad, stream in such case.
	alGenBuffers(1, &sn.buffer);
    alBufferData(sn.buffer, sn.audio_format, data, (ALsizei)data_size, sample_rate);

    alGenSources(1, &sn.source);
	alSourcef(sn.source, AL_PITCH, 1);
	alSourcef(sn.source, AL_GAIN, 1.0f);
	alSource3f(sn.source, AL_POSITION, 0, 0, 0);
	alSource3f(sn.source, AL_VELOCITY, 0, 0, 0);
	alSourcei(sn.source, AL_LOOPING, bits & AU_LOOP_BIT);
	alSourcei(sn.source, AL_BUFFER, sn.buffer);

    al_check_error();
    
    return sparse_push(Au_table.sounds, sn);
}

void au_set_listener_pos(vec3 pos) {
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}

vec3 au_get_listener_pos() {
    vec3 pos = {};
    alGetListener3f(AL_POSITION, &pos.x, &pos.y, &pos.z);
    return pos;
}

void au_play_sound(u16 sound) {
    const auto &sn = Au_table.sounds[sound];
    alSourcePlay(sn.source);
    al_check_error();
}

void au_play_sound_or_continue(u16 sound) {
    const auto &sn = Au_table.sounds[sound];

    s32 state;
    alGetSourcei(sn.source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(sn.source);
    }

    al_check_error();
}

void au_play_sound_or_continue(u16 sound, vec3 location) {
    const auto &sn = Au_table.sounds[sound];
    alSource3f(sn.source, AL_POSITION, location.x, location.y, location.z);

    au_play_sound_or_continue(sound);
}

void au_stop_sound(u16 sound) {
    const auto &sn = Au_table.sounds[sound];
    alSourceStop(sn.source);

    al_check_error();
}
