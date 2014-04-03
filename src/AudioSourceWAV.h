#ifndef WAVFILESRC_H_
#define WAVFILESRC_H_

class AudioSourceWAV : public AudioDataSource
{
	SNDFILE*  mWavFile;
	SF_INFO *info;
	uint32 mChannels;
	uint32 mRate;

public:
	AudioSourceWAV();
	~AudioSourceWAV();

	bool Open(const char* Filename);
	uint32 Read(void* buffer, size_t count);
	void Seek(float Time);
	size_t GetLength();
	uint32 GetRate();
	uint32 GetChannels();
	bool IsValid();
};


#endif