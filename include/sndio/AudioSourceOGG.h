#pragma once

class AudioSourceOGG : public AudioDataSource
{
    class AudioSourceOGGInternal;

    std::unique_ptr<AudioSourceOGGInternal> internal;

    float mSeekTime;

    bool mIsValid;
    bool mIsDataLeft;

public:
    AudioSourceOGG();
    ~AudioSourceOGG();
    bool open(std::filesystem::path Filename) override;
    uint32_t read(short* buffer, size_t count) override;
    void seek(float Time) override;
    size_t get_length() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t get_rate() override; // Returns sampling rate of audio
    uint32_t get_channels() override; // Returns channels of audio
    bool is_valid() override;
    bool has_data_left() override;
};