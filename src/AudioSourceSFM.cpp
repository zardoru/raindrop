
#include <cstdio>
#include <sndfile.h>
#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"


#define RIFF_MAGIC 0x46464952
#define RIFF_MAGIC_ALT 0x66666972
#define WAV_MAGIC 0x45564157
#define FMT_MAGIC 0x20746D66
#define FMT_MAGIC_ALT 0x00746D66
#define DATA_MAGIC 0x61746164

struct wavHeader
{
	int riff_head;
	int fsize;
	int wav_head;
	int fmt_mark;
	int data_size;
	short int data_type;
	short int chan_cnt;
	int sample_rate;
	int sbc;
	short int align;
	short int bits_per_sample;
	int data_mark;
	int data_chunk_size;
};

bool validate_wav_header(FILE* f, wavHeader& head)
{
	fread(&head, sizeof(head), 1, f);

	if (head.riff_head != RIFF_MAGIC && head.riff_head != RIFF_MAGIC_ALT)
		return false;

	if (head.wav_head != WAV_MAGIC)
		return false;

	if (head.fmt_mark != FMT_MAGIC && head.fmt_mark != FMT_MAGIC_ALT)
		return false;

	if (head.data_mark != DATA_MAGIC)
		return false;

	if (head.data_type != 1) // uncompressed
		return false;

	return true;
}

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
	if (info->format & SF_FORMAT_WAV)
		return info->frames * 2;
	else
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