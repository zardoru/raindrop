#include <cstdio>
#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceWAV.h"

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

AudioSourceWAV::AudioSourceWAV()
{
	mWavFile = NULL;
}

AudioSourceWAV::~AudioSourceWAV()
{
	if (mWavFile)
		fclose(mWavFile);
}

bool AudioSourceWAV::Open(const char* Filename)
{
	wavHeader Header;

	mWavFile = fopen(Filename, "rb");

	if (!mWavFile)
		return false;

	if (!validate_wav_header(mWavFile, Header))
	{
		fclose (mWavFile);
		mWavFile = NULL;
		return false;
	}

	mDataLength = Header.data_size / 8;
	mRate		= Header.sample_rate;
	mChannels   = Header.chan_cnt;
	mDataChunkSize = Header.data_chunk_size;

	return true;
}

uint32 AudioSourceWAV::Read(void* buffer, size_t count)
{
	if (mWavFile)
		return fread(buffer, mDataLength, count, mWavFile);
	return 0;
}

void AudioSourceWAV::Seek(float Time)
{
	if (mWavFile)
		fseek(mWavFile, sizeof(wavHeader) + Time * mRate, SEEK_SET);
}

size_t AudioSourceWAV::GetLength()
{
	return mDataChunkSize / mDataLength;
}

uint32 AudioSourceWAV::GetRate()
{
	return mRate;
}

uint32 AudioSourceWAV::GetChannels()
{
	return mChannels;
}

bool AudioSourceWAV::IsValid()
{
	return mWavFile != NULL;
}