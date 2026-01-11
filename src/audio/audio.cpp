#include "pch.h"
#include "audio_player.h"
#include "wav.h"

Audio_Player *get_audio_player   () { Assert(audio_player); return audio_player; }
Vector3 get_audio_listener_position () { return get_audio_player()->listener_position; }
void set_audio_listener_position (Vector3 position) { get_audio_player()->listener_position = position; }

Parsed_Wav parse_wav(void *data) {
    auto header = *Eat(&data, Wav_Header);
    
    if (make_string(header.riff_id, 4) != S("RIFF")) {
		log(LOG_ERROR, "File is not a valid wav file, header does not begin with 'RIFF'");
		return {};
	}

    if (make_string(header.wave_id, 4) != S("WAVE")) {
		log(LOG_ERROR, "File is not a valid wav file, header does not begin with 'WAVE'");
		return {};
	}
     
    if (make_string(header.fmt_id, 4) != S("fmt ")) {
        log(LOG_ERROR, "File is not a valid wav file, header does not contain 'fmt '");
        return {};
    }

    if (make_string(header.data_id, 4) == S("data")) {
        return { header, data };
    }

    if (make_string(header.data_id, 4) == S("LIST")) {
        const u32 list_size = header.sampled_data_size;
        eat(&data, list_size);

        char *data_id = Eat(&data, char, 4);
        if (make_string(data_id, 4) == S("data")) {
            copy(header.data_id, data_id, 4);
            header.sampled_data_size = *Eat(&data, u32);
            return { header, data };
        }
    }
        
    log(LOG_ERROR, "File is not a valid wav file, header does not contain 'data'");
    return {};
}
