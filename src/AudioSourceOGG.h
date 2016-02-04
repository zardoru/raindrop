#pragma once

class AudioSourceOGG : public AudioDataSource
{
    OggVorbis_File mOggFile;

    vorbis_info* info;
    vorbis_comment* comment;
    float mSeekTime;

    bool mIsValid;
    bool mIsDataLeft;

public:
    AudioSourceOGG();
    ~AudioSourceOGG();
    bool Open(const char* Filename) override;
    uint32_t Read(short* buffer, size_t count) override;
    void Seek(float Time) override;
    size_t GetLength() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t GetRate() override; // Returns sampling rate of audio
    uint32_t GetChannels() override; // Returns channels of audio
    bool IsValid() override;
    bool HasDataLeft() override;
};