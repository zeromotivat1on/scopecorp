#include "pch.h"
#include "al.h"
#include "alc.h"
#include "audio_player.h"
#include "wav.h"
#include "vector.h"
#include "file_system.h"

void update_audio() {
    auto player = get_audio_player();
    auto listener_position = player->listener_position;

	alListener3f(AL_POSITION, listener_position.x, listener_position.y, listener_position.z);
}

static void al_check_error(Source_Code_Location loc = __location) {
    ALenum al_error = alGetError();
    while (al_error != AL_NO_ERROR) {
        log(LOG_MINIMAL, "OpenAL error 0x%X at %s:%d", al_error, loc.file, loc.line);
        al_error = alGetError();
    }
}

static Audio_Player *audio_player = null;

bool init_audio_player() {
    Assert(!audio_player);

    auto player = audio_player = New(Audio_Player);
    
    auto device = alcOpenDevice(null);
	if (!device) {
		log(LOG_MINIMAL, "Failed to open default audio device");
        return false;
	}

    player->audio_device._p = device;
    
    auto context = alcCreateContext(device, null);
	if (!context) {
		log(LOG_MINIMAL, "Failed to create alc context");
        return false;
	}

    player->audio_context._p = context;
    
	if (!alcMakeContextCurrent(context)) {
		log(LOG_MINIMAL, "Failed to make alc context current");
        return false;
	}

    table_realloc(player->sound_table, 64);

    add_directory_files(&player->sound_catalog, PATH_SOUND(""), true);
    
    return true;
}

static s32 to_al_audio_format(s32 channel_count, s32 bit_rate) {
	if (channel_count == 1 && bit_rate == 8)       return AL_FORMAT_MONO8;
	else if (channel_count == 1 && bit_rate == 16) return AL_FORMAT_MONO16;
	else if (channel_count == 2 && bit_rate == 8)  return AL_FORMAT_STEREO8;
	else if (channel_count == 2 && bit_rate == 16) return AL_FORMAT_STEREO16;
	else {
		log(LOG_MINIMAL, "Unknown audio format with channel count %d and bit rate %d", channel_count, bit_rate);
		return 0;
	}
}

Sound *new_sound(String path) {
    auto contents = read_file(path, __temporary_allocator);
    return new_sound(path, contents);
}

Sound *new_sound(String path, Buffer contents) {
    if (!is_valid(contents)) return null;

    auto wav = parse_wav(contents.data);
    if (!wav.sampled_data) return null;

    path = copy_string(path);
    auto name = get_file_name_no_ext(path);

    auto player = get_audio_player();
    auto &sound = player->sound_table[name];

    sound.path          = path;
    sound.name          = name;
    sound.bits          = 0;
    sound.channel_count = wav.header.channel_count;
    sound.bit_rate      = wav.header.bits_per_sample;
    sound.sample_rate   = wav.header.samples_per_second;
    sound.data_size     = wav.header.sampled_data_size;
    sound.audio_format  = to_al_audio_format(sound.channel_count, sound.bit_rate);
    
    alGenBuffers(1, &sound.buffer._u32);
    alGenSources(1, &sound.source._u32);

    alBufferData(sound.buffer._u32, sound.audio_format, wav.sampled_data, sound.data_size, sound.sample_rate);
    
    alSourcef (sound.source._u32, AL_PITCH,    1);
    alSourcef (sound.source._u32, AL_GAIN,     1.0f);
    alSource3f(sound.source._u32, AL_POSITION, 0, 0, 0);
    alSource3f(sound.source._u32, AL_VELOCITY, 0, 0, 0);
    alSourcei (sound.source._u32, AL_LOOPING,  0);
    alSourcei (sound.source._u32, AL_BUFFER,   sound.buffer._u32);
        
    al_check_error();

    return &sound;
}

Sound *get_sound(String name) {
    return table_find(get_audio_player()->sound_table, name);
}

void play_sound(const Sound *sound) {
    Vector3 pos;
    alGetListener3f(AL_POSITION, &pos.x, &pos.y, &pos.z);
    play_sound(sound, pos);
}

void play_sound(const Sound *sound, Vector3 position) {
    alSource3f(sound->source._u32, AL_POSITION, position.x, position.y, position.z);

    s32 state;
    alGetSourcei(sound->source._u32, AL_SOURCE_STATE, &state);
    
    if (state != AL_PLAYING) {
        alSourcePlay(sound->source._u32);
    }

    al_check_error();
}

void stop_sound(const Sound *sound) {
    alSourceStop(sound->source._u32);
    al_check_error();
}

void set_looping(Sound *sound, bool loop) {
    if (loop) {
        sound->bits &=  SOUND_LOOP_BIT;
    } else {
        sound->bits |= ~SOUND_LOOP_BIT;        
    }

    alSourcei(sound->source._u32, AL_LOOPING, loop);
}

void play_sound        (String name)            { play_sound(get_sound(name)); }
void play_sound        (String name, Vector3 p) { play_sound(get_sound(name), p); }
void stop_sound        (String name)            { stop_sound(get_sound(name)); }
void set_sound_looping (String name, bool loop) { set_looping(get_sound(name), loop); }
