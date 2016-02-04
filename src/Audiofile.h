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
    virtual bool Open(const char* Filename) = 0;
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
    virtual bool Open(const char* Filename) = 0;
    virtual void Play() = 0;
    virtual bool IsPlaying() = 0;
    virtual void SeekTime(float Second) = 0;
    virtual void SeekSample(uint32_t Sample) = 0;
    virtual void Stop() = 0;
    void SetPitch(double pitch);
    double GetPitch();
    void SetLoop(bool Loop);
    bool IsLooping();
    uint32_t GetChannels();
};

class AudioSample : public Sound
{
    uint32_t	 mRate;
    uint32_t   mCounter;
    float    mAudioStart, mAudioEnd;
    std::shared_ptr<std::vector<short>> mData;
    bool	 mValid;
    bool	 mIsPlaying;
    bool	 mIsValid;

public:
    AudioSample();
    AudioSample(AudioSample& Other);
    AudioSample(AudioSample &&Other);
    ~AudioSample();
    uint32_t Read(float* buffer, size_t count) override;
    bool Open(const char* Filename) override;
    bool Open(AudioDataSource* Source);
    void Play() override;
    void SeekTime(float Second) override;
    void SeekSample(uint32_t Sample) override;
    void Stop() override;

    bool IsPlaying() override;
    void Slice(float audio_start, float audio_end);
    std::shared_ptr<AudioSample> CopySlice();
    // void Mix(AudioSample& Other);
    bool IsValid();
};

class AudioStream : public Sound
{
    PaUtilRingBuffer mRingBuf;

    std::unique_ptr<AudioDataSource> mSource;
    unsigned int     mBufferSize;
    std::vector<short>	 mData;
    std::vector<short>	 mResampleBuffer;
    std::vector<short>	 mOutputBuffer;
    short			 tbuf[8192];
    double			 mStreamTime;
    double			 mPlaybackTime;

    bool			 mIsPlaying;
    soxr_t			 mResampler;

public:
    AudioStream();
    ~AudioStream();

    uint32_t Read(float* buffer, size_t count) override;
    bool Open(const char* Filename) override;
    void Play() override;
    void SeekTime(float Second) override;
    void SeekSample(uint32_t Sample) override;
    void Stop() override;

    double GetStreamedTime();
    double GetPlayedTime();
    uint32_t GetRate();

    uint32_t Update();
    bool IsPlaying() override;
};
