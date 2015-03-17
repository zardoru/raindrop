
#include <sndfile.h>
#include <soxr.h>

#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"
#include "AudioSourceOGG.h"

#ifdef MP3_ENABLED
#include "AudioSourceMP3.h"
#endif

#include <boost/algorithm/string.hpp>

// Buffer -> buffer to convert to stereo (interleaved) cnt -> current samples max_len -> Maximum samples
void monoToStereo(short* Buffer, size_t cnt, size_t max_len)
{
	if (cnt <= max_len/2) // We're within boundaries.
	{
		// Okay, we have to transform all the samples from 0, 1, 2, 3...
		// to 0, 2, 4, 6... We start at the end so we don't have anything to overwrite.
		// We also need, after that is done, to copy all samples 0, 2, 4, 6 into 1, 3, 5, 7 and so on.
		// So interleave them...
		for (size_t i = cnt - 1; i >= 0; i--)
		{
			Buffer[i * 2] = Buffer[i];
		}

		// Now copy 0 into 1, 2 into 3, and so on.
		for (size_t i = 1; i < cnt * 2; i += 2)
		{
			Buffer[i] = Buffer[i - 1];
		}
	}
}

AudioDataSource* SourceFromExt(Directory Filename)
{
	AudioDataSource *Ret = NULL;
	GString Ext = Filename.GetExtension();

	if (Filename.path().length() == 0 || Ext.length() == 0) {
		wprintf(L"Invalid filename. (%s) (%s)\n", Utility::Widen(Filename).c_str(), Utility::Widen(Ext).c_str());
		return NULL;
	}

	boost::algorithm::to_lower(Ext);

	const char* xt = Ext.c_str();
	if (strstr(xt, "wav") || strstr(xt, "flac"))
		Ret = new AudioSourceSFM();
#ifdef MP3_ENABLED
	else if (strstr(xt, "mp3"))
		Ret = new AudioSourceMP3();
#endif
	else if (strstr(xt, "ogg"))
		Ret = new AudioSourceOGG();

	if (Ret)
		Ret->Open(Filename.c_path());
	else
		wprintf(L"extension %ls has no audiosource associated\n", Utility::Widen(Ext).c_str());

	return Ret;
}

void Sound::SetPitch(double Pitch)
{
	mPitch = Pitch;
}

double Sound::GetPitch()
{
	return mPitch;
}

void Sound::SetLoop(bool Loop)
{
	mIsLooping = Loop;
}

bool Sound::IsLooping()
{
	return mIsLooping;
}

uint32 Sound::GetChannels()
{
	return Channels;
}

AudioSample::AudioSample()
{
	mPitch = 1;
	mIsPlaying = false;
	mIsValid = false;
	mIsLooping = false;
	mData = NULL;
}

AudioSample::~AudioSample()
{
	delete mData;
	mData = NULL;
}

bool AudioSample::Open(AudioDataSource* Src)
{
	if (Src && Src->IsValid())
	{
		Channels = Src->GetChannels();
		mBufferSize = Src->GetLength() * Channels;

		if (!mBufferSize) // Huh what why?
			return false;

		mData = new short[mBufferSize];
		size_t total = Src->Read(mData, mBufferSize);

		if (total < mBufferSize) // Oh, odd. Oh well.
			mBufferSize = total;

		

		mRate = Src->GetRate();

		if (Channels == 1) // Mono? We'll need to duplicate information for both channels.
		{
			size_t size = mBufferSize * 2;
			short* mDataNew = new short[size];

			for (size_t i = 0, j = 0; i < mBufferSize; i++, j += 2)
			{
				mDataNew[j] = mData[i];
				mDataNew[j + 1] = mData[i];
			}

			delete mData;
			mBufferSize = size;
			mData = mDataNew;
			Channels = 2;
		}

		if (mRate != 44100) // mixer_constant.. in the future, allow changing this destination sample rate.
		{
			size_t done;
			size_t doneb;
			double ResamplingRate = 44100.0 / mRate;
			soxr_io_spec_t spc;
			size_t size = size_t(double(mBufferSize * ResamplingRate + .5));
			short* mDataNew = new short[size];

			spc.e = 0;
			spc.itype = SOXR_INT16_I;
			spc.otype = SOXR_INT16_I;
			spc.scale = 1;
			spc.flags = 0;

			soxr_oneshot(mRate, 44100, Channels,
				mData, mBufferSize / Channels, &done,
				mDataNew, size / Channels, &doneb,
				&spc, NULL, NULL);

			delete mData;
			mBufferSize = size;
			mData = mDataNew;
		}

		mCounter = 0;
		mIsValid = true;
		mByteSize = mBufferSize * sizeof(short);

		return true;
	}
	return false;
}

uint32 AudioSample::Read(short* buffer, size_t count)
{
	if (!mIsPlaying)
		return 0;

	if (mIsValid)
	{
		size_t bufferLeft = mBufferSize-mCounter;
		uint32 ReadAmount = min(bufferLeft, count);

		if (mCounter < mBufferSize)
		{	
			int diff = 0;

			memcpy(buffer, mData+mCounter, ReadAmount * sizeof(short));

			mCounter += ReadAmount;
		}else
		{
			mIsPlaying = false;
			memset(buffer, 0, count);
		}

		return count;
	}else
		return 0;

}

bool AudioSample::IsPlaying()
{
	return mIsPlaying;
}

GString RearrangeFilename(const char* Fn)
{
	GString Ret;
	Directory File = Fn;
	File.Normalize();
	if (Utility::FileExists(File.c_path()))
		return Fn;
	else
	{
		GString Ext = File.GetExtension();
		boost::algorithm::to_lower(Ext);

		if (strstr(Ext.c_str(), "wav"))
			Ret = Utility::RemoveExtension(Fn) + ".ogg";
		else
			Ret = Utility::RemoveExtension(Fn) + ".wav";

		if (!Utility::FileExists(Ret))
			return File.c_path();
		else
			return Ret;
	}
}

bool AudioSample::Open(const char* Filename)
{
	GString FilenameFixed = RearrangeFilename(Filename);

	AudioDataSource * Src = SourceFromExt (FilenameFixed);

	bool Success = Open(Src);

	delete Src;
	return Success;
}

void AudioSample::Play()
{
	mIsPlaying = true;
	mCounter = 0;
}

void AudioSample::SeekTime(float Second)
{
	mCounter = mRate * Second;

	if (mCounter >= mBufferSize)
		mCounter = mBufferSize;
}

void AudioSample::SeekSample(uint32 Sample)
{
	mCounter = Sample;

	if (mCounter >= mBufferSize)
		mCounter = mBufferSize;
}

void AudioSample::Stop()
{
	mIsPlaying = false;
}

AudioStream::AudioStream()
{
	mPitch = 1;
	mIsPlaying = false;
	mIsLooping = false;
	mSource = NULL;
	mData = NULL;
	tmpBuffer = NULL;
}

AudioStream::~AudioStream()
{
	delete mSource;
	delete[] mData;
	delete tmpBuffer;
}

uint32 AudioStream::Read(short* buffer, size_t count)
{
	size_t cnt;
	size_t toRead = count; // Count is the amount of s16 samples.
	size_t outcnt;

	if (!mSource || !mSource->IsValid())
	{
		mIsPlaying = false;
		return 0;
	}

	if (Channels == 1) // We just want half the samples.
		toRead >>= 1;
	
	if (PaUtil_GetRingBufferReadAvailable(&mRingBuf) < toRead || !mIsPlaying)
	{
		memset(buffer, 0, toRead);
		toRead = PaUtil_GetRingBufferReadAvailable(&mRingBuf);
	}

	if (mIsPlaying)
	{
		if (mSource->GetRate() != 44100.0) // Alright, we'll need to resample. Let's just do that.
		{
			// This is what our destination rate will be
			double origRate = mSource->GetRate();

			// This is what our destination rate is.
			double resRate = 44100.0 / mPitch;

			// This is how many samples we want to read from the source buffer
			size_t scount = ceil(origRate * toRead / resRate);

			cnt = PaUtil_ReadRingBuffer(&mRingBuf, tmpBuffer, scount);
			// cnt now contains how many samples we actually read... 

			// This is how many resulting samples we can output with what we read...
			outcnt = (cnt * resRate / origRate);

			soxr_io_spec_t sis;
			sis.flags = 0;
			sis.itype = SOXR_INT16_I;
			sis.otype = SOXR_INT16_I;
			sis.scale = 1;

			size_t done;

			soxr_oneshot(origRate, resRate, Channels,
				tmpBuffer, cnt / Channels, NULL,
				buffer, outcnt / Channels, &done,
				&sis, NULL, NULL);

			if (Channels == 1)
				monoToStereo(tmpBuffer, cnt, BUFF_SIZE);
		}
		else
		{
			cnt = PaUtil_ReadRingBuffer(&mRingBuf, buffer, toRead);

			if (Channels == 1)
				monoToStereo(buffer, cnt, BUFF_SIZE);

			outcnt = cnt;
		}

		mStreamTime += (double)(cnt/Channels) / (double)mSource->GetRate();
		mPlaybackTime = mStreamTime - MixerGetLatency();
	}else
		return 0;

	return outcnt ? outcnt : mIsPlaying;
}

bool AudioStream::Open(const char* Filename)
{
	mSource = SourceFromExt(RearrangeFilename(Filename));

	if (mSource && mSource->IsValid())
	{
		Channels = mSource->GetChannels();
		
		if (mSource->GetRate() != 44100) // Okay, we'll rearrange a buffer to store resampling stuff meanwhile.
			tmpBuffer = new short[BUFF_SIZE]; 
		

		mBufferSize = BUFF_SIZE;

		mData = new short[mBufferSize];
		PaUtil_InitializeRingBuffer(&mRingBuf, sizeof(int16), mBufferSize, mData);

		mStreamTime = mPlaybackTime = 0;
		
		SeekTime(0);

		return true;
	}else
		return false;
}

bool AudioStream::IsPlaying()
{
	return mIsPlaying;
}

void AudioStream::Play()
{
	mIsPlaying = true;
}

void AudioStream::SeekTime(float Second)
{
	if (mSource)
		mSource->Seek(Second);
	mStreamTime = Second;
}

double AudioStream::GetStreamedTime()
{
	return mStreamTime;
}

double AudioStream::GetPlayedTime()
{
	return mPlaybackTime;
}

void AudioStream::SeekSample(uint32 Sample)
{
	mSource->Seek((float)Sample / (float)mSource->GetRate());
}

void AudioStream::Stop()
{
	mIsPlaying = false;
}

uint32 AudioStream::Update()
{
	short tbuf[BUFF_SIZE];
	uint32 eCount = PaUtil_GetRingBufferWriteAvailable(&mRingBuf);
	uint32 ReadTotal;

	if (!mSource) return 0;

	mSource->SetLooping(IsLooping());

	if (ReadTotal = mSource->Read(tbuf, eCount))
	{
		PaUtil_WriteRingBuffer(&mRingBuf, tbuf, ReadTotal);
	}else
	{
		if (!PaUtil_GetRingBufferReadAvailable(&mRingBuf) && !mSource->HasDataLeft())
			mIsPlaying = false;
	}

	return ReadTotal;
}

uint32 AudioStream::GetRate()
{
	return mSource->GetRate();
}

AudioDataSource::AudioDataSource()
{
}

AudioDataSource::~AudioDataSource()
{
}

void AudioDataSource::SetLooping(bool Loop)
{
	mSourceLoop = Loop;
}
