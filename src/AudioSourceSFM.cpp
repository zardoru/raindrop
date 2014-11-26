
#ifdef WIN32
#include <Windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif

#include <cstdio>
#include <sndfile.h>
#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"

AudioSourceSFM::AudioSourceSFM()
{
	mWavFile = NULL;
	info = NULL;
	mIsDataLeft = false;
}

AudioSourceSFM::~AudioSourceSFM()
{
	if (mWavFile)
		sf_close(mWavFile);
	if (info)
		delete info;
}

bool AudioSourceSFM::Open(const char* Filename)
{
	info = new SF_INFO;
	info->format = 0;


#ifndef WIN32
	mWavFile = sf_open(Filename, SFM_READ, info);
#else
	mWavFile = sf_wchar_open( Utility::Widen(Filename).c_str(), SFM_READ, info);
#endif

#ifndef NDEBUG
	dFILENAME = Filename;
#endif

	mRate		= info->samplerate;
	mChannels   = info->channels;

	int err = 0;
	if (!mWavFile || (err = sf_error(mWavFile)))
	{
		wprintf(L"Error %d: Filename %ls (wavfile %p)\n", err, Utility::Widen(Filename).c_str(), mWavFile);
		return false;
	}

	sf_command(mWavFile, SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);
	mIsDataLeft = true;
	return true;
}

uint32 AudioSourceSFM::Read(short* buffer, size_t count)
{
	uint32 read;
	if (mWavFile)
	{
		read = sf_read_short(mWavFile, buffer, count);
		int remaining = count - read;

		while (mSourceLoop && (remaining > 0))
		{
			Seek(0);
			read += sf_read_short(mWavFile, (short*)(buffer) + (read), remaining);
			remaining -= read;
		}

		
		if (read <= 0)
			mIsDataLeft = false;
	}

	return read;
}

void AudioSourceSFM::Seek(float Time)
{
	if (mWavFile)
	{
		mIsDataLeft = true;
		sf_seek(mWavFile, Time * mRate, SEEK_SET);
	}
}

size_t AudioSourceSFM::GetLength()
{
	return info->frames;
}

uint32 AudioSourceSFM::GetRate()
{
	return mRate;
}

uint32 AudioSourceSFM::GetChannels()
{
	return mChannels;
}

bool AudioSourceSFM::IsValid()
{
	return mWavFile != NULL;
}

bool AudioSourceSFM::HasDataLeft()
{
	return mIsDataLeft;
}
