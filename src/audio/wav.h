#pragma once

struct Wav_Header {
    char riff_id[4];
    u32  file_size;
    char wave_id[4];
    
    char fmt_id[4];
    u32 chunk_size;         // 16, 18 or 40
    u16 format_tag;         // 1 - PCM int, 3 - IEEE 754 float, 6 - 8alaw, 7 - 8mulaw
    u16 channel_count;      // 1 - mono, 2 - stereo
    u32 samples_per_second; // sample rate in Hz
    u32 bytes_per_second;   // frequency * bytes_per_block
    u16 bytes_per_block;    // channel_count * bits_per_sample / 8
    u16 bits_per_sample;

    // Next chunk can be 'data' or 'LIST' or other.
    // But as we are interested in 'data' only, 'LIST' or other chunks
    // will be parsed and skipped in favor of 'data' only.
    
    char data_id[4];
    u32 sampled_data_size;
};

struct Parsed_Wav {
    Wav_Header header;
    
    // Sampled data goes here, basically an offset pointer from given data in parse_wav.
    // This is also the field you can check to ensure parsing was finished correctly.
    void *sampled_data = null;
};

Parsed_Wav parse_wav (void *data);
