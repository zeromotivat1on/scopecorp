#include "pch.h"
#include "audio/au_table.h"
#include "audio/au_sound.h"
#include "audio/wav.h"

#include "log.h"
#include "str.h"
#include "memory_eater.h"

void au_create_table(Au_Table &t) {
    reserve(t.arena, MB(1));
    
    sparse_reserve(t.arena, t.sounds, t.MAX_SOUNDS);
    
    // Add dummies at 0 index.
    sparse_push(t.sounds);
}

void au_destroy_table(Au_Table &t) {
    release(t.arena);
    t = {};
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
        eat(&data, list_size);

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
