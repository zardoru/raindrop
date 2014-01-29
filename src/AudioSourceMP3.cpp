#ifdef MP3_ENABLED
#include "Global.h"
#include "Audio.h"
#include "AudioSourceMP3.h"

bool mpg123_initialized = false;

AudioSourceMP3::AudioSourceMP3()
{
	int err;

	if (!mpg123_initialized)
	{
		mpg123_init();
	}

	mHandle = mpg123_new(NULL, &err);
	mIsValid = false;
}

AudioSourceMP3::~AudioSourceMP3()
{ 
	mpg123_close(mHandle);
	mpg123_delete(mHandle);
}

bool AudioSourceMP3::Open(const char* Filename)
{
	if (mpg123_open(mHandle, Filename) == MPG123_OK)
	{
		long rate;
		mpg123_getformat(mHandle, &rate, &mChannels, &mEncoding);

		mRate = rate;

		mpg123_format_none(mHandle);
		mpg123_format(mHandle, mRate, mChannels, mEncoding);

		size_t pos = mpg123_tell(mHandle);
		size_t start = mpg123_seek(mHandle, 0, SEEK_SET);
		size_t end = mpg123_seek(mHandle, 0, SEEK_END);
		mpg123_seek(mHandle, pos, SEEK_SET);

		mLen = end - start;

		mIsValid = true;
		return true;
	}

	return false;
}

uint32 AudioSourceMP3::Read(void* buffer, size_t count)
{
	size_t ret;
	int res = mpg123_read(mHandle, (unsigned char*)buffer, count * sizeof(int16), &ret);
	return res != MPG123_DONE;
}

void AudioSourceMP3::Seek(float Time)
{
	mpg123_seek(mHandle, mRate * Time * mChannels, SEEK_SET);
}

size_t AudioSourceMP3::GetLength()
{
	return mLen;
}

uint32 AudioSourceMP3::GetRate()
{ 
	return mRate;
}

uint32 AudioSourceMP3::GetChannels()
{ 
	return mChannels;
}

bool AudioSourceMP3::IsValid()
{
	return mIsValid;
}

#endif