#pragma once

class AudioSourceSFM : public AudioDataSource
{
	SNDFILE*  mWavFile;
	SF_INFO *info;
	uint32 mChannels;
	uint32 mRate;
	uint32 mFlen;
	bool mIsDataLeft;

public:
	AudioSourceSFM();
	~AudioSourceSFM();

	bool Open(const char* Filename) override;
	uint32 Read(short* buffer, size_t count) override;
	void Seek(float Time) override;
	size_t GetLength() override;
	uint32 GetRate() override;
	uint32 GetChannels() override;
	bool IsValid() override;
	bool HasDataLeft() override;
};