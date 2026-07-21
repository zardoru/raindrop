#pragma once

struct SF_INFO;

#include "Interruptible.h"

class AudioSourceOJM : public AudioDataSource, Interruptible
{
    static const int OJM_OGG = 1;
    static const int OJM_WAV = 2;

    struct
    {
        int enabled;
        void *info;
        void* file;
    } TemporaryState;

    std::shared_ptr<AudioSample> arr_[2000];
    std::shared_ptr<std::ifstream> ifile;
    void parseM30();
    void parse_omc();

    double Speed;
public:
    explicit AudioSourceOJM(Interruptible* parent = nullptr);
    ~AudioSourceOJM() override;
    bool open(std::filesystem::path filename) override;
    std::shared_ptr<AudioSample> get_from_index(int index);
    void seek(float time) override;
    uint32_t read(short* buffer, size_t count) override;

    size_t get_length() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t get_rate() override; // Returns sampling rate of audio
    uint32_t get_channels() override; // Returns channels of audio
    bool is_valid() override;
    bool has_data_left() override;
    void set_pitch(double speed);
};