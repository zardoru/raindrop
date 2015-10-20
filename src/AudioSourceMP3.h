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
	bool Open(const char* Filename) override;
	uint32 Read(short* buffer, size_t count) override;
	void Seek(float Time) override;
	size_t GetLength() override; // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate() override; // Returns sampling rate of audio
	uint32 GetChannels() override; // Returns channels of audio
	bool IsValid() override;
	bool HasDataLeft() override;
};

#endif

#endif