#include "pch.h"
#include "audio/al.h"
#include "audio/alc.h"
#include "audio/sound.h"

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

void init_audio_context() {
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

static s32 al_audio_format(s32 channel_count, s32 bit_rate) {
	if (channel_count == 1 && bit_rate == 8)       return AL_FORMAT_MONO8;
	else if (channel_count == 1 && bit_rate == 16) return AL_FORMAT_MONO16;
	else if (channel_count == 2 && bit_rate == 8)  return AL_FORMAT_STEREO8;
	else if (channel_count == 2 && bit_rate == 16) return AL_FORMAT_STEREO16;
	else {
		error("Unknown audio format with channel count %d and bit rate %d", channel_count, bit_rate);
		return 0;
	}
}

void init_sound_asset(Sound *sound, void *data) {
    sound->audio_format = al_audio_format(sound->channel_count, sound->bit_rate);

    // @Cleanup: loading big sounds like that is bad, stream in such case.
	alGenBuffers(1, &sound->buffer);
    alBufferData(sound->buffer, sound->audio_format, data, (ALsizei)sound->data_size, sound->sample_rate);

    alGenSources(1, &sound->source);
	alSourcef(sound->source, AL_PITCH, 1);
	alSourcef(sound->source, AL_GAIN, 1.0f);
	alSource3f(sound->source, AL_POSITION, 0, 0, 0);
	alSource3f(sound->source, AL_VELOCITY, 0, 0, 0);
	alSourcei(sound->source, AL_LOOPING, sound->flags & SOUND_FLAG_LOOP);
	alSourcei(sound->source, AL_BUFFER, sound->buffer);

    al_check_error();
}

void set_listener_pos(vec3 pos) {
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}

void play_sound(sid sid) {
    const auto &sound = asset_table.sounds[sid];
    alSourcePlay(sound.source);
    al_check_error();
}

void play_sound_or_continue(sid sid) {
    const auto &sound = asset_table.sounds[sid];

    s32 state;
    alGetSourcei(sound.source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(sound.source);
    }

    al_check_error();
}

void play_sound_or_continue(sid sid, vec3 location) {
    const auto &sound = asset_table.sounds[sid];
    alSource3f(sound.source, AL_POSITION, location.x, location.y, location.z);

    play_sound_or_continue(sid);
}

void stop_sound(sid sid) {
    const auto &sound = asset_table.sounds[sid];
    alSourceStop(sound.source);

    al_check_error();
}
