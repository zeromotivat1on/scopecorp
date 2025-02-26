#include "pch.h"
#include "sound.h"
#include "log.h"
#include "os/file.h"
#include "audio/al.h"
#include "profile.h"
#include "memory_eater.h"
#include <stdio.h>
#include <string.h>

void load_game_sounds(Sound_List* list) {
    // @Cleanup:: create_sound loads sound data directly to sound card memory,
    // so its bad to load big files in such case (they should be streamed then).
    // But its good for small, frequently used sounds like player steps.
    
    list->world = create_sound(DIR_SOUNDS "C418_Shuniji.wav", false);
    list->player_steps = create_sound(DIR_SOUNDS "player_steps.wav", true);
    list->player_steps_cute = create_sound(DIR_SOUNDS "player_steps_cute.wav", true);
}

static s32 al_determine_audio_format(s32 channel_count, s32 bit_depth) {
    if (channel_count == 1 && bit_depth == 8)       return AL_FORMAT_MONO8;
    else if (channel_count == 1 && bit_depth == 16) return AL_FORMAT_MONO16;
    else if (channel_count == 2 && bit_depth == 8)  return AL_FORMAT_STEREO8;
    else if (channel_count == 2 && bit_depth == 16) return AL_FORMAT_STEREO16;
    else {
        error("Unknown audio format, channel count (%d), bit depth (%d)", channel_count, bit_depth);
        return 0;
    }
}

Sound create_sound(const char* path, bool loop) {
    char timer_string[256];
    sprintf_s(timer_string, sizeof(timer_string), "%s from %s took", __FUNCTION__, path);
    SCOPE_TIMER(timer_string);
    
    Sound sound;
    sound.path = path;
    alGenBuffers(1, &sound.buffer);

    u64 sound_file_size;
    char* sound_data = (char*)read_entire_file_temp(path, &sound_file_size);

    s32 sound_data_size;
    sound_data = (char*)extract_wav(sound_data, &sound.channel_count, &sound.sample_rate, &sound.bit_depth, &sound_data_size);
    if (!sound_data) return {0};

    sound.audio_format = al_determine_audio_format(sound.channel_count, sound.bit_depth);

    // @Cleanup: loading big sounds like that is bad, stream in such case.
    alBufferData(sound.buffer, sound.audio_format, sound_data, sound_data_size, sound.sample_rate);

    // We've copied buffer data to sound card memory, so free it on cpu.
    free_temp(sound_file_size);

    //if ((al_error = alGetError()) != AL_NO_ERROR) log("al_error (0x%X)", al_error);
    
    alGenSources(1, &sound.source);
    alSourcef(sound.source, AL_PITCH, 1);
    alSourcef(sound.source, AL_GAIN, 1.0f);
    alSource3f(sound.source, AL_POSITION, 0, 0, 0);
    alSource3f(sound.source, AL_VELOCITY, 0, 0, 0);
    alSourcei(sound.source, AL_LOOPING, loop);
    alSourcei(sound.source, AL_BUFFER, sound.buffer);
    
    return sound;
}

void* extract_wav(void* data, s32* channel_count, s32* sample_rate, s32* bits_per_sample, s32* size) {   
    const char* riff = (char*)eat(&data, 4);
    if (strncmp(riff, "RIFF", 4) != 0) {
        error("File is not a valid wave file, header does not begin with RIFF");
        return null;
    }

    eat_s32(&data); // file size

    const char* wave_str = (char*)eat(&data, 4);
    if (strncmp(wave_str, "WAVE", 4) != 0) {
        error("File is not a valid wave file, header does not contain WAVE");
        return null;
    }

    const char* fmt_str = (char*)eat(&data, 4);
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

    const char* data_str = (char*)eat(&data, 4);
    if (strncmp(data_str, "data", 4) != 0) {
        error("File is not a valid wave file, header does not contain DATA");
        return null;
    }

    if (size) *size = eat_s32(&data);
    
    return data;
}
