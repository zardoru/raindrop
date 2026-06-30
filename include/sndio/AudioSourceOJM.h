#pragma once

struct SF_INFO;

#include "Interruptible.h"

class AudioSourceOJM : public AudioDataSource, Interruptible
{
    static const int OJM_OGG = 1;
    static const int OJM_WAV = 2;

    struct
    {
        int Enabled;
        void *Info;
        void* File;
    } TemporaryState;

    std::shared_ptr<AudioSample> Arr[2000];
    std::shared_ptr<std::ifstream> ifile;
    void parseM30();
    void parseOMC();

    double Speed;
public:
    AudioSourceOJM(Interruptible* Parent = nullptr);
    ~AudioSourceOJM();
    bool open(std::filesystem::path Filename) override;
    std::shared_ptr<AudioSample> GetFromIndex(int Index);
    void seek(float Time) override;
    uint32_t read(short* buffer, size_t count) override;

    size_t get_length() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t get_rate() override; // Returns sampling rate of audio
    uint32_t get_channels() override; // Returns channels of audio
    bool is_valid() override;
    bool has_data_left() override;
    void SetPitch(double speed);
};