#include "pch.h"
#include "audio/al.h"
#include "audio/alc.h"
#include "audio/sound.h"

#include "log.h"
#include "profile.h"
#include "memory_eater.h"
#include "memory_storage.h"
#include "stb_sprintf.h"

#include "os/file.h"

#include <string.h>

void init_audio_context()
{
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

static s32 al_determine_audio_format(s32 channel_count, s32 bit_depth) {
	if (channel_count == 1 && bit_depth == 8)       return AL_FORMAT_MONO8;
	else if (channel_count == 1 && bit_depth == 16) return AL_FORMAT_MONO16;
	else if (channel_count == 2 && bit_depth == 8)  return AL_FORMAT_STEREO8;
	else if (channel_count == 2 && bit_depth == 16) return AL_FORMAT_STEREO16;
	else {
		error("Unknown audio format with channel count %d and bit depth %d", channel_count, bit_depth);
		return 0;
	}
}

// Extract wave header data and return pointer to start of actual sound data.
static void *extract_wav(void *data, s32 *channel_count, s32 *sample_rate, s32 *bits_per_sample, s32 *size) {
	const char *riff = (char *)eat(&data, 4);
	if (strncmp(riff, "RIFF", 4) != 0) {
		error("File is not a valid wave file, header does not begin with RIFF");
		return null;
	}

	eat_s32(&data); // file size

	const char *wave_str = (char *)eat(&data, 4);
	if (strncmp(wave_str, "WAVE", 4) != 0) {
		error("File is not a valid wave file, header does not contain WAVE");
		return null;
	}

	const char *fmt_str = (char *)eat(&data, 4);
	if (strncmp(fmt_str, "fmt", 3) != 0) {
		error("File is not a valid wave file, header does not contain FMT");
		return null;
	}

	const s32 fmt_size = eat_s32(&data);
	if (fmt_size != 16) {
		error("File is not a valid wave file, fmt size is not 16");
		return null;
	}

	eat_s16(&data); // audio format

	if (channel_count) *channel_count = eat_s16(&data);
	if (sample_rate) *sample_rate = eat_s32(&data);

	eat_s32(&data); // byte rate
	eat_s16(&data); // block align

	if (bits_per_sample) *bits_per_sample = eat_s16(&data);

	const char *data_str = (char *)eat(&data, 4);
	if (strncmp(data_str, "data", 4) != 0) {
		error("File is not a valid wave file, header does not contain DATA");
		return null;
	}

	if (size) *size = eat_s32(&data);

	return data;
}

Sound create_sound(const char *path, u32 flags) {
	char timer_string[256];
	stbsp_snprintf(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
	SCOPE_TIMER(timer_string);

	Sound sound;
	sound.path = path;
	alGenBuffers(1, &sound.buffer);

    u64 sound_file_size;
	char *sound_data = (char *)push(temp, MAX_SOUND_SIZE);
    defer { pop(temp, MAX_SOUND_SIZE); };
    
    if (!read_file(path, sound_data, MAX_SOUND_SIZE, &sound_file_size)) return {0};

	s32 sound_data_size;
	sound_data = (char *)extract_wav(sound_data, &sound.channel_count, &sound.sample_rate, &sound.bit_depth, &sound_data_size);
	if (!sound_data) return { 0 };

	sound.audio_format = al_determine_audio_format(sound.channel_count, sound.bit_depth);

	// @Cleanup: loading big sounds like that is bad, stream in such case.
	alBufferData(sound.buffer, sound.audio_format, sound_data, sound_data_size, sound.sample_rate);

	//if ((al_error = alGetError()) != AL_NO_ERROR) log("al_error (0x%X)", al_error);

	alGenSources(1, &sound.source);
	alSourcef(sound.source, AL_PITCH, 1);
	alSourcef(sound.source, AL_GAIN, 1.0f);
	alSource3f(sound.source, AL_POSITION, 0, 0, 0);
	alSource3f(sound.source, AL_VELOCITY, 0, 0, 0);
	alSourcei(sound.source, AL_LOOPING, flags & SOUND_FLAG_LOOP);
	alSourcei(sound.source, AL_BUFFER, sound.buffer);

	return sound;
}
