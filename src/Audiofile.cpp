
#include <sndfile.h>
#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"
#include "AudioSourceOGG.h"

#ifdef MP3_ENABLED
#include "AudioSourceMP3.h"
#endif

#include <boost/algorithm/string.hpp>

AudioDataSource* SourceFromExt(Directory Filename)
{
	AudioDataSource *Ret = NULL;
	GString Ext = Filename.GetExtension();

	if (Filename.path().length() == 0 || Ext.length() == 0) {
		wprintf(L"Invalid filename. (%s) (%s)\n", Filename.c_path(), Ext.c_str());
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
	mIsPlaying = false;
	mIsValid = false;
	mIsLooping = false;
	mData = NULL;
}

AudioSample::~AudioSample()
{
	if (mData) delete mData;
}

bool AudioSample::Open(AudioDataSource* Src)
{
	if (Src && Src->IsValid())
	{
		Channels = Src->GetChannels();
		mBufferSize = Src->GetLength() * Channels;

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
		}

		if (mRate != 44100) // mixer_constant.. in the future, resample better.
		{
			double ResamplingRate = 44100.0 / (double)mRate;
			size_t size = size_t(double(mBufferSize * ResamplingRate));
			short* mDataNew = new short[size];

			size_t i;
			double j;
			for (j = i = 0; i < mBufferSize && j < size; i++, j += ResamplingRate)
			{
				int dst = j;
				mDataNew[dst] = mData[i];

				if (i > 0)
				{
					// linearly interpolate
					int start = (i - 1)*ResamplingRate;
					double step = double(mDataNew[dst] - mDataNew[start]) / double(dst - start);
					for (int k = (start + 1); k < dst; k++)
					{
						mDataNew[k] = step * (dst - k) + mDataNew[start];
					}
				}
			}

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

	if (Utility::FileExists(Fn))
		return Fn;
	else
	{
		GString Ext = Directory(Fn).GetExtension();
		boost::algorithm::to_lower(Ext);

		if (strstr(Ext.c_str(), "wav"))
			Ret = Utility::RemoveExtension(Fn) + ".ogg";
		else
			Ret = Utility::RemoveExtension(Fn) + ".wav";

		if (!Utility::FileExists(Ret))
			return Fn;
		else
			return Ret;
	}
}

bool AudioSample::Open(const char* Filename)
{
	GString FilenameFixed = RearrangeFilename(Filename);

	AudioDataSource * Src = SourceFromExt (FilenameFixed);

	bool Success = Open(Src);

	if (Src)
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
	mIsPlaying = false;
	mIsLooping = false;
	mSource = NULL;
	mData = NULL;
}

AudioStream::~AudioStream()
{
	if (mSource)
		delete mSource;

	if (mData)
		delete[] mData;
}

uint32 AudioStream::Read(short* buffer, size_t count)
{
	size_t cnt;
	size_t toRead = count; // Count is the amount of s16 samples.

	if (!mSource || !mSource->IsValid())
	{
		mIsPlaying = false;
		return 0;
	}
	
	if (PaUtil_GetRingBufferReadAvailable(&mRingBuf) < toRead || !mIsPlaying)
	{
		memset(buffer, 0, toRead);
		toRead = PaUtil_GetRingBufferReadAvailable(&mRingBuf);
	}

	if (mIsPlaying)
	{
		cnt = PaUtil_ReadRingBuffer(&mRingBuf, buffer, toRead);
		mStreamTime += (double)(cnt/Channels) / (double)mSource->GetRate();
		mPlaybackTime = mStreamTime - MixerGetLatency();
	}else
		return 0;

	return cnt ? cnt : mIsPlaying;
}

bool AudioStream::Open(const char* Filename)
{
	mSource = SourceFromExt(Filename);

	if (mSource && mSource->IsValid())
	{
		Channels = mSource->GetChannels();

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
