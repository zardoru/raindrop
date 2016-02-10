#pragma once

class AudioSourceSFM : public AudioDataSource
{
    SNDFILE*  mWavFile;
    SF_INFO *info;
    uint32_t mChannels;
    uint32_t mRate;
    uint32_t mFlen;
    bool mIsDataLeft;

public:
    AudioSourceSFM();
    ~AudioSourceSFM();

    bool Open(std::filesystem::path Filename) override;
    uint32_t Read(short* buffer, size_t count) override;
    void Seek(float Time) override;
    size_t GetLength() override;
    uint32_t GetRate() override;
    uint32_t GetChannels() override;
    bool IsValid() override;
    bool HasDataLeft() override;
};