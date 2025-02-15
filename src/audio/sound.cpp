#include "pch.h"
#include "sound.h"
#include "file.h"
#include "audio/al.h"
#include "profile.h"
#include <stdio.h>

void load_game_sounds(Sound_List* list)
{
    list->world = create_sound(DIR_SOUNDS "2814_Recovery.wav");
}

Sound create_sound(const char* path)
{
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

    if (sound.channel_count == 1 && sound.bit_depth == 8)       sound.audio_format = AL_FORMAT_MONO8;
    else if (sound.channel_count == 1 && sound.bit_depth == 16) sound.audio_format = AL_FORMAT_MONO16;
    else if (sound.channel_count == 2 && sound.bit_depth == 8)  sound.audio_format = AL_FORMAT_STEREO8;
    else if (sound.channel_count == 2 && sound.bit_depth == 16) sound.audio_format = AL_FORMAT_STEREO16;
    else
    {
        log("Unknown wave audio format");
        return {0};
    }

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
    alSourcei(sound.source, AL_LOOPING, AL_FALSE);
    alSourcei(sound.source, AL_BUFFER, sound.buffer);

    // alGetSourcei(source, AL_SOURCE_STATE, &state);
    
    return sound;
}
