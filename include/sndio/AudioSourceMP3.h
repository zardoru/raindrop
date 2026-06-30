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
    bool open(std::filesystem::path Filename) override;
    uint32_t read(short* buffer, size_t count) override;
    void seek(float Time) override;
    size_t get_length() override; // Always returns total samples. Frames = Length/Channels.
    uint32_t get_rate() override; // Returns sampling rate of audio
    uint32_t get_channels() override; // Returns channels of audio
    bool is_valid() override;
    bool has_data_left() override;

	struct Metadata {
		std::string artist;
		std::string title;
	};

	Metadata GetMetadata();
};
