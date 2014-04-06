
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

	mWavFile = sf_open(Filename, SFM_READ, info);

	mRate		= info->samplerate;
	mChannels   = info->channels;

	int err = 0;
	if (!mWavFile || (err = sf_error(mWavFile)))
	{
		printf("Error %d (wavfile %p)\n", err, mWavFile);
		return false;
	}

	return true;
}

uint32 AudioSourceSFM::Read(void* buffer, size_t count)
{
	uint32 cnt;
	if (mWavFile)
	{
		cnt = sf_read_short(mWavFile, (short*)buffer, count);
		int remaining = count - cnt;

		while (mSourceLoop && (remaining > 0))
		{
			Seek(0);
			cnt += sf_read_short(mWavFile, (short*)(buffer) + cnt, remaining);
			remaining -= cnt;
		}

	}
	return cnt;
}

void AudioSourceSFM::Seek(float Time)
{
	if (mWavFile)
		sf_seek(mWavFile, Time * mRate / mChannels, SEEK_SET);
}

size_t AudioSourceSFM::GetLength()
{
	// I'm not sure why- but this is inconsistent.
	return info->frames * mChannels;
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