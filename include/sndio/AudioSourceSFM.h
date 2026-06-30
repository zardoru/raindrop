#pragma once

struct SF_INFO;

class AudioSourceSFM : public AudioDataSource
{
    void*  mWavFile;
    SF_INFO *info;
    uint32_t mChannels;
    uint32_t mRate;
    uint32_t mFlen;
    bool mIsDataLeft;

public:
    AudioSourceSFM();
    ~AudioSourceSFM();

    bool open(std::filesystem::path Filename) override;
    uint32_t read(short* buffer, size_t count) override;
    void seek(float Time) override;
    size_t get_length() override;
    uint32_t get_rate() override;
    uint32_t get_channels() override;
    bool is_valid() override;
    bool has_data_left() override;
};