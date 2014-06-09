#ifndef WAVFILESRC_H_
#define WAVFILESRC_H_

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

	bool Open(const char* Filename);
	uint32 Read(short* buffer, size_t count);
	void Seek(float Time);
	size_t GetLength();
	uint32 GetRate();
	uint32 GetChannels();
	bool IsValid();
	bool HasDataLeft();
};


#endif