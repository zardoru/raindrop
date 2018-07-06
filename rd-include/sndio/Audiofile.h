#pragma once

class AudioDataSource
{
protected:
    bool mSourceLoop;

#ifndef NDEBUG
    std::string dFILENAME;
#endif

public:
    AudioDataSource();
    virtual ~AudioDataSource();
    virtual bool Open(std::filesystem::path Filename) = 0;
    virtual uint32_t Read(short* buffer, size_t count) = 0; // count is in samples.
    virtual void Seek(float Time) = 0;
    virtual size_t GetLength() = 0; // Always returns total frames.
    virtual uint32_t GetRate() = 0; // Returns sampling rate of audio
    virtual uint32_t GetChannels() = 0; // Returns channels of audio
    virtual bool IsValid() = 0;
    virtual bool HasDataLeft() = 0;

    void SetLooping(bool Loop);
};

class Sound
{
protected:
    uint32_t Channels;
    bool mIsLooping;
    double mPitch;
public:
    virtual ~Sound() = default;
    virtual uint32_t Read(float* buffer, size_t count) = 0;
    virtual bool Open(std::filesystem::path Filename) = 0;
    virtual void Play() = 0;
    virtual bool IsPlaying() = 0;
    virtual void SeekTime(float Second) = 0;
    virtual void SeekSample(uint32_t Sample) = 0;
    virtual void Stop() = 0;
    void SetPitch(double pitch);
    double GetPitch() const;
    void SetLoop(bool Loop);
    bool IsLooping() const;
    uint32_t GetChannels() const;
};

class IMixer;

class AudioSample : public Sound
{
    uint32_t	 mRate;
    uint32_t   mCounter;
    float    mAudioStart, mAudioEnd;
    std::shared_ptr<std::vector<short>> mData;
    bool	 mIsPlaying;
    std::atomic<bool> mIsValid;
	std::atomic<bool> mIsLoaded;
	std::future<bool> mThread;
    IMixer *mOwnerMixer;
public:
    AudioSample();
    AudioSample(IMixer* owner_mixer);
    AudioSample(const AudioSample& Other);
    AudioSample(AudioSample &&Other);
    ~AudioSample();
	void Seek(size_t offs);
	uint32_t Read(float* buffer, size_t count) override;
    bool Open(std::filesystem::path Filename) override;
    bool Open(std::filesystem::path Filename, bool async);
    bool Open(AudioDataSource* Source, bool async = false);
    void Play() override;
    void SeekTime(float Second) override;
    void SeekSample(uint32_t Sample) override;
    void Stop() override;

	bool AwaitLoad();

	// returns duration in seconds
	double GetDuration();

    bool IsPlaying() override;
    void Slice(float audio_start, float audio_end);
    std::shared_ptr<AudioSample> CopySlice();
    // void Mix(AudioSample& Other);
    bool IsValid() const;
};

class AudioStream : public Sound
{
    class AudioStreamInternal;

    std::unique_ptr<AudioStreamInternal> internal;
    

    std::unique_ptr<AudioDataSource> mSource;
    unsigned int     mBufferSize;
    std::vector<short>	 mData;
    std::vector<short>	 mResampleBuffer;
    std::vector<short>	 mOutputBuffer;
    short			 tbuf[8192];
    double			 mStreamTime;
    double			 mPlaybackTime;

    bool			 mIsPlaying;
    IMixer *mOwnerMixer;
public:
    AudioStream();
    AudioStream(IMixer* owner_mixer);
    ~AudioStream();

	double mStreamStartTime;

    uint32_t Read(float* buffer, size_t count) override;
    bool Open(std::filesystem::path Filename) override;
    void Play() override;
    void SeekTime(float Second) override;
    void SeekSample(uint32_t Sample) override;
    void Stop() override;

    double GetStreamedTime() const;
    double GetPlayedTime() const;

	// return time that has been played according to the DAC clock
	// based off current mixer time and mStreamStartTime, assigned by audio.cpp
	double GetPlayedTimeDAC() const;

    uint32_t GetRate() const;

    uint32_t Update();
    bool IsPlaying() override;
};
