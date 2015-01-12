#ifdef MP3_ENABLED

#ifndef MP3FILESRC_H_
#define MP3FILESRC_H_

#ifndef MINGW
#include <mpg123.h>
#else
#include <mpg123mingw.h>
#endif

class AudioSourceMP3 : public AudioDataSource
{
	mpg123_handle *mHandle;
	uint32 mRate;
	int mEncoding;
	int mChannels;
	size_t mLen;

	bool mIsValid;
	bool mIsDataLeft;

public:
	AudioSourceMP3();
	~AudioSourceMP3();
	bool Open(const char* Filename);
	uint32 Read(short* buffer, size_t count);
	void Seek(float Time);
	size_t GetLength(); // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate(); // Returns sampling rate of audio
	uint32 GetChannels(); // Returns channels of audio
	bool IsValid();
	bool HasDataLeft();
};

#endif

#endif