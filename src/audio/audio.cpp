#include "pch.h"
#include "audio/audio_registry.h"
#include "audio/sound.h"
#include "audio/wav.h"

#include "log.h"
#include "sid.h"
#include "str.h"
#include "memory_eater.h"

void init_audio_registry() {
    audio_registry.sounds = Sparse_Array<Sound>(MAX_SOUNDS);
}

void cache_sound_sids(Sound_Sid_List *list) {
	list->world             = SID("/data/sounds/wind_ambience.wav");
    list->player_steps      = SID("/data/sounds/player_steps.wav");
    list->player_steps_cute = SID("/data/sounds/player_steps_cute.wav");
}

void *parse_wav(void *data, Wav_Header *header) {
    Wav_Header wavh = *(Wav_Header *)eat(&data, sizeof(Wav_Header));
    if (!str_cmp(wavh.riff_id, "RIFF", 4)) {
		error("File is not a valid wav file, header does not begin with 'RIFF'");
		return null;
	}

    if (!str_cmp(wavh.wave_id, "WAVE", 4)) {
		error("File is not a valid wav file, header does not begin with 'WAVE'");
		return null;
	}
     
    if (!str_cmp(wavh.fmt_id, "fmt ", 4)) {
		error("File is not a valid wav file, header does not contain 'fmt '");
		return null;
	}

    // If we found 'data' chunk, we are done, but ...
    if (str_cmp(wavh.data_id, "data", 4)) {
        if (header) *header = wavh;
        return data;
	}

    // ... it may be possible that we have 'LIST' chunk instead of 'data', so parse it.
    if (str_cmp(wavh.data_id, "LIST", 4)) {
        const u32 list_size = wavh.sampled_data_size;
        eat(&data, list_size); // 

        const char *data_id = (char *)eat(&data, 4);
        if (str_cmp(data_id, "data", 4)) {
            str_copy(wavh.data_id, data_id, 4);
            wavh.sampled_data_size = eat_u32(&data);

            if (header) *header = wavh;
            return data;
        }
    }
        
    error("File is not a valid wav file, header does not contain 'data'");
    return null;
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

	if (size) {
        if (channel_count && *channel_count == 1) {
            *size = eat_big_endian_s32(&data);
        } else if (channel_count && *channel_count == 2) {
            *size = eat_s32(&data);
        }
    }

	return data;
}
