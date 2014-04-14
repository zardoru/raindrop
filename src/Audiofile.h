#ifndef AUDIOFILE_H_
#define AUDIOFILE_H_

#include "pa_ringbuffer.h"

class AudioDataSource 
{
protected:
	bool mSourceLoop;
public:
	virtual ~AudioDataSource();
	virtual bool Open(const char* Filename) = 0;
	virtual uint32 Read(void* buffer, size_t count) = 0;
	virtual void Seek(float Time) = 0;
	virtual size_t GetLength() = 0; // Always returns total frames.
	virtual uint32 GetRate() = 0; // Returns sampling rate of audio
	virtual uint32 GetChannels() = 0; // Returns channels of audio
	virtual bool IsValid() = 0;

	void SetLooping(bool Loop);
};

class Sound
{
protected:
	bool mIsLooping;
	uint32 Channels;
public:
	virtual uint32 Read(void* buffer, size_t count) = 0;
	virtual bool Open(const char* Filename) = 0;
	virtual void Play() = 0;
	virtual bool IsPlaying() = 0;
	virtual void SeekTime(float Second) = 0;
	virtual void SeekSample(uint32 Sample) = 0;
	virtual void Stop() = 0;
	void SetLoop(bool Loop);
	bool IsLooping();
	uint32 GetChannels();
};


class AudioSample : public Sound
{
private:

	short* mData;
	unsigned int   mBufferSize;
	unsigned int   mCounter;
	bool		   mValid;
	bool		   mIsPlaying;
	bool		   mIsValid;
	uint32		   mRate;
	uint32		   mByteSize;


public:
	AudioSample();
	~AudioSample();
	uint32 Read(void* buffer, size_t count);
	bool Open(const char* Filename);
	void Play();
	void SeekTime(float Second);
	void SeekSample(uint32 Sample);
	void Stop();

	bool IsPlaying();
};

class AudioStream : public Sound
{
private:

	AudioDataSource* mSource;
	short*   mData;
	unsigned int     mBufferSize;
	PaUtilRingBuffer mRingBuf;
	bool			 mIsPlaying;
	float			 mStreamTime;
	float			 mPlaybackTime;

public:
	AudioStream();
	~AudioStream();

	uint32 Read(void* buffer, size_t count);
	bool Open(const char* Filename);
	void Play();
	void SeekTime(float Second);
	void SeekSample(uint32 Sample);
	void Stop();

	float GetStreamedTime();
	float GetPlayedTime();
	uint32 GetRate();

	uint32 Update();
	bool IsPlaying();

};

#endif