#ifdef MP3_ENABLED

#include "Global.h"
#include "Logging.h"
#include "Audio.h"
#include "AudioSourceMP3.h"

static bool mpg123_initialized = false;

AudioSourceMP3::AudioSourceMP3()
{
	int err;

	if (!mpg123_initialized)
	{
		mpg123_init();
		mpg123_initialized = true;
	}

	mHandle = mpg123_new(NULL, &err);
	mpg123_format_none(mHandle);
	mIsValid = false;
	mIsDataLeft = false;
}

AudioSourceMP3::~AudioSourceMP3()
{ 
	mpg123_close(mHandle);
	mpg123_delete(mHandle);
}

bool AudioSourceMP3::Open(const char* Filename)
{
	mpg123_param(mHandle, MPG123_FORCE_RATE, 44100, 1);

#ifndef NDEBUG
	dFILENAME = Filename;
#endif

#if !(defined WIN32) || (defined MINGW)
	if (mpg123_open(mHandle, Filename) == MPG123_OK)
#else
	if (mpg123_topen(mHandle, Utility::Widen(Filename).c_str()) == MPG123_OK)
#endif
	{
		long rate;

		mpg123_format_all(mHandle);
		mpg123_format(mHandle, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);

		mpg123_getformat(mHandle, &rate, &mChannels, &mEncoding);

		mRate = rate;

		size_t pos = mpg123_tell(mHandle);
		size_t start = mpg123_seek(mHandle, 0, SEEK_SET);
		size_t end = mpg123_seek(mHandle, 0, SEEK_END);
		mpg123_seek(mHandle, pos, SEEK_SET);

		mLen = end - start;

		mIsValid = true;
		mIsDataLeft = true;
		return true;
	}

	return false;
}

uint32 AudioSourceMP3::Read(short* buffer, size_t count)
{
	size_t ret;
	size_t toRead = count * sizeof(short);

	if (toRead == 0)
		return 0;

	int res = mpg123_read(mHandle, (unsigned char*)buffer, toRead, &ret);

	if (res == MPG123_DONE || ret < toRead)
	{
		if (!mSourceLoop)
			mIsDataLeft = false;
		else
		{
			Seek(0);

			size_t place = 0;
			if (res > 0)
			{
				count -= ret / sizeof(short);
				place = ret / sizeof(short);
			}

			Read(buffer + place, count);
		}
	}

	// according to mpg123_read documentation, ret is the amount of bytes read. We want to return samples read.
	return ret / sizeof(short);
}

void AudioSourceMP3::Seek(float Time)
{
	mIsDataLeft = true;
	off_t place = mRate * Time;
	int res = mpg123_seek(mHandle, place, SEEK_SET);
	if (res < 0 || res < place)
	{
		Log::Printf("Error seeking stream at %f (tried %d, got %d)\n", Time, place, res);
	}

	if (place > mLen)
		Log::Printf("Attempt to seek after the stream's end.\n");
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

bool AudioSourceMP3::HasDataLeft()
{
	return mIsDataLeft;
}

#endif
