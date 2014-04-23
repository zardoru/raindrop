#ifdef MP3_ENABLED

#ifndef MP3FILESRC_H_
#define MP3FILESRC_H_

#include <mpg123.h>

class AudioSourceMP3 : public AudioDataSource
{
	bool mIsValid;
	mpg123_handle *mHandle;
	uint32 mRate;
	int mEncoding;
	int mChannels;
	uint32 mLen;
	bool mIsDataLeft;

public:
	AudioSourceMP3();
	~AudioSourceMP3();
	bool Open(const char* Filename);
	uint32 Read(void* buffer, size_t count);
	void Seek(float Time);
	size_t GetLength(); // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate(); // Returns sampling rate of audio
	uint32 GetChannels(); // Returns channels of audio
	bool IsValid();
	bool HasDataLeft();
};

#endif

#endif