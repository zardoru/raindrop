#ifndef WAVFILESRC_H_
#define WAVFILESRC_H_

#include <cstdio>

class AudioSourceWAV : public AudioDataSource
{
	bool   mIsValid;
	FILE*  mWavFile;
	uint32 mChannels;
	uint32 mRate;
	uint32 mDataLength;
	uint32 mDataChunkSize;
	
	unsigned char* mData;

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