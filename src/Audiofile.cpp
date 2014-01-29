#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceOGG.h"
#include "AudioSourceWAV.h"

#ifdef MP3_ENABLED
#include "AudioSourceMP3.h"
#endif

AudioDataSource* SourceFromExt(String Filename)
{
	AudioDataSource *Ret;

	if (Utility::GetExtension(Filename) == "wav")
		Ret = new AudioSourceWAV();
	else if (Utility::GetExtension(Filename) == "ogg")
		Ret = new AudioSourceOGG();
#ifdef MP3_ENABLED
	else if (Utility::GetExtension(Filename) == "mp3")
		Ret = new AudioSourceMP3();
#endif

	Ret->Open(Filename.c_str());
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
			if(count > bufferLeft)
				count = bufferLeft;

			memcpy(buffer, (char*)mData+mCounter, count);

			mCounter += count;
		}else
		{
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

bool AudioSample::Open(const char* Filename)
{
	AudioDataSource * Src = SourceFromExt (Filename);

	if (Src->IsValid())
	{
		Channels = Src->GetChannels();
		mBufferSize = Src->GetLength() * sizeof(uint16) * Channels;

		mData = new unsigned char[mBufferSize];
		Src->Read(mData, mBufferSize / Channels);

		mRate = Src->GetRate();
		mCounter = 0;
		mIsValid = true;
		return true;
	}

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

uint32 AudioStream::Read(void* buffer, size_t count)
{
	size_t cnt;
	size_t toRead = count;
	
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

	if (mSource->IsValid())
	{
		Channels = mSource->GetChannels();

		mBufferSize = BUFF_SIZE;

		mData = new unsigned char[sizeof(int16) * mBufferSize];
		PaUtil_InitializeRingBuffer(&mRingBuf, sizeof(int16), mBufferSize, mData);

		mStreamTime = mPlaybackTime = 0;
		Update();
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
	unsigned char tbuf[BUFF_SIZE * 2];
	uint32 eCount = PaUtil_GetRingBufferWriteAvailable(&mRingBuf);
	uint32 ReadTotal;

	mSource->SetLooping(IsLooping());

	if (ReadTotal = mSource->Read(tbuf, eCount))
	{
		PaUtil_WriteRingBuffer(&mRingBuf, tbuf, eCount);
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