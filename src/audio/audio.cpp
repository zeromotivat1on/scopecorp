#include "pch.h"
#include "audio/audio_registry.h"
#include "audio/sound.h"

#include "log.h"
#include "sid.h"
#include "str.h"
#include "memory_eater.h"

void init_audio_registry() {
    audio_registry.sounds = Sparse_Array<Sound>(MAX_SOUNDS);
}

void cache_sound_sids(Sound_Sid_List *list) {
	list->world             = SID("/data/sounds/C418_Shuniji.wav");
    list->player_steps      = SID("/data/sounds/player_steps.wav");
    list->player_steps_cute = SID("/data/sounds/player_steps_cute.wav");
}

void *extract_wav(void *data, s32 *channel_count, s32 *sample_rate, s32 *bits_per_sample, u32 *size) {
	const char *riff = (char *)eat(&data, 4);
	if (str_cmp(riff, "RIFF")) {
		error("File is not a valid wave file, header does not begin with RIFF");
		return null;
	}

	eat_s32(&data); // file size

	const char *wave_str = (char *)eat(&data, 4);
	if (str_cmp(wave_str, "WAVE")) {
		error("File is not a valid wave file, header does not contain WAVE");
		return null;
	}

	const char *fmt_str = (char *)eat(&data, 4);
	if (str_cmp(fmt_str, "fmt")) {
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
	if (sample_rate)   *sample_rate   = eat_s32(&data);

	eat_s32(&data); // byte rate
	eat_s16(&data); // block align

	if (bits_per_sample) *bits_per_sample = eat_s16(&data);

	const char *data_str = (char *)eat(&data, 4);
	if (str_cmp(data_str, "data")) {
		error("File is not a valid wave file, header does not contain DATA");
		return null;
	}

	if (size) *size = eat_s32(&data);

	return data;
}
