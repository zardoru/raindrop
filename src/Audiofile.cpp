
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

AudioDataSource* SourceFromExt(String Filename)
{
	AudioDataSource *Ret = NULL;
	String Ext = Utility::GetExtension(Filename);

	if (Filename.length() == 0) return NULL;

	boost::algorithm::to_lower(Ext);

	if (Ext == "wav" || Ext == "flac")
		Ret = new AudioSourceSFM();
#ifdef MP3_ENABLED
	else if (Ext == "mp3")
		Ret = new AudioSourceMP3();
#endif
	else if (Ext == "ogg")
		Ret = new AudioSourceOGG();

	if (Ret)
		Ret->Open(Filename.c_str());
	else
		printf("INFO: extension %s has no audiosource associated\n", Ext.c_str());

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

uint32 AudioSample::Read(void* buffer, size_t count)
{
	if (!mIsPlaying)
		return 0;

	if (mIsValid)
	{
		uint32 bufferLeft = mBufferSize-mCounter;
		
		count *= sizeof(int16);

		if (mCounter < mBufferSize)
		{	
			int diff = 0;
			if(count > bufferLeft)
			{
				diff = count - bufferLeft;
				count = bufferLeft;
			}

			memcpy(buffer, (char*)mData+mCounter, count);

			memset((char*)buffer + count, 0, diff);

			mCounter += count;
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

String RearrangeFilename(const char* Fn)
{
	String Ret;

	if (Utility::FileExists(Fn))
		return Fn;
	else
	{
		String Ext = Utility::GetExtension(Fn);

		if (Ext == "wav")
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
	String FilenameFixed = RearrangeFilename(Filename);

	AudioDataSource * Src = SourceFromExt (FilenameFixed);

	if (Src && Src->IsValid())
	{
		Channels = Src->GetChannels();
		mBufferSize = Src->GetLength() * Channels;

		mData = new short[mBufferSize];
		Src->Read(mData, mBufferSize);

		mRate = Src->GetRate();

		if (mRate != 44100) // mixer_constant.. in the future, resample.
		{
			printf("AudioSample::Open(): Sample rate (%d) != System Sample Rate (44100)\n", mRate); 
			
			double ResamplingRate = 44100.0 / (double)mRate;
			short* mDataNew = new short [int(double(mBufferSize * ResamplingRate))];

			int i;
			double j;
			for (j = i = 0; i < mBufferSize; i++, j += ResamplingRate)
			{
				int dst = j;
				mDataNew[dst] = mData[i];

				if (i > 0)
				{
					// linearly interpolate
					int start = (i-1)*ResamplingRate;
					double step = double (mDataNew[dst] - mDataNew[start]) / double (dst - start);
					for (int k = (start + 1); k < dst; k++)
					{
						mDataNew[k] = step * (dst - k) + mDataNew[start];
					}
				}
			}

			delete mData;
			mBufferSize *= ResamplingRate;
			mData = mDataNew;
		}

		mCounter = 0;
		mIsValid = true;

		delete Src;
		return true;
	}else
		printf("Invalid source for %s.\n", Filename);

	delete Src;
	return false;
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

uint32 AudioStream::Read(void* buffer, size_t count)
{
	size_t cnt;
	size_t toRead = count; // Count is the amount of s16 samples.

	if (!mSource)
		return 0;
	
	if (PaUtil_GetRingBufferReadAvailable(&mRingBuf) < toRead || !mIsPlaying)
	{
		memset(buffer, 0, toRead);
		toRead = PaUtil_GetRingBufferReadAvailable(&mRingBuf);
	}

	if (mIsPlaying)
	{
		cnt = PaUtil_ReadRingBuffer(&mRingBuf, buffer, toRead);
		mStreamTime += (double)(cnt/Channels) / (double)mSource->GetRate();
		mPlaybackTime = mStreamTime - GetDeviceLatency();
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
	mSource->Seek(Second);
}

float AudioStream::GetStreamedTime()
{
	return mStreamTime;
}

float AudioStream::GetPlayedTime()
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
		if (!PaUtil_GetRingBufferReadAvailable(&mRingBuf))
			mIsPlaying = false;
	}

	return ReadTotal;
}

uint32 AudioStream::GetRate()
{
	return mSource->GetRate();
}


AudioDataSource::~AudioDataSource()
{
}

void AudioDataSource::SetLooping(bool Loop)
{
	mSourceLoop = Loop;
}
