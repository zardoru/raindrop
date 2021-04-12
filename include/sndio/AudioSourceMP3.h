#pragma once

class AudioSourceMP3 : public AudioDataSource
{
    void *mHandle;
    void *ioHandle;
    uint32_t mRate;
    int mEncoding;
    int mChannels;
    size_t mLen;

    bool mIsValid;
    bool mIsDataLeft;

public:
    AudioSourceMP3();
    ~AudioSourceMP3();
    bool Open(std::filesystem::path Filename) override;
    uint32_t Read(short* buffer, size_t count) override;
    void Seek(float Time) override;
    size_t GetLength() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t GetRate() override; // Returns sampling rate of audio
    uint32_t GetChannels() override; // Returns channels of audio
    bool IsValid() override;
    bool HasDataLeft() override;

	struct Metadata {
		std::string artist;
		std::string title;
	};

	Metadata GetMetadata();
};
