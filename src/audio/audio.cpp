#include "pch.h"
#include "audio/au_table.h"
#include "audio/au_sound.h"
#include "audio/wav.h"

#include "log.h"
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
    if (!str_equal(String { wavh.riff_id, 4 }, S("RIFF"))) {
		error("File is not a valid wav file, header does not begin with 'RIFF'");
		return null;
	}

    if (!str_equal(String { wavh.wave_id, 4 }, S("WAVE"))) {
		error("File is not a valid wav file, header does not begin with 'WAVE'");
		return null;
	}
     
    if (!str_equal(String { wavh.fmt_id, 4 }, S("fmt "))) {
        error("File is not a valid wav file, header does not contain 'fmt '");
        return null;
    }

    // If we found 'data' chunk, we are done, but ...
    if (str_equal(String { wavh.data_id, 4 }, S("data"))) {
        if (header) *header = wavh;
        return data;
    }

    // ... it may be possible that we have 'LIST' chunk instead of 'data', so parse it.
    if (str_equal(String { wavh.data_id, 4 }, S("LIST"))) {
        const u32 list_size = wavh.sampled_data_size;
        eat(&data, list_size);

        char *data_id = (char *)eat(&data, 4);
        if (str_equal(String { data_id, 4 }, S("data"))) {
            mem_copy(wavh.data_id, data_id, 4);
            wavh.sampled_data_size = eat_u32(&data);

            if (header) *header = wavh;
            return data;
        }
    }
        
    error("File is not a valid wav file, header does not contain 'data'");
    return null;
}
