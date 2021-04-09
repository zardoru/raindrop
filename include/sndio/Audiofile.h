#pragma once


class AudioDataSource
{
protected:
    bool mSourceLoop;
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
    virtual bool IsPlaying() const = 0;
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

    bool IsPlaying() const override;
    void Slice(float audio_start, float audio_end);
    std::shared_ptr<AudioSample> CopySlice();
    // void Mix(AudioSample& Other);
    bool IsValid() const;
};

struct stream_time_map_t {
    double clock_start, clock_end;
    int64_t frame_start, frame_end;
    inline double map(double clock, double sample_rate) {
        double t_relative = (clock - clock_start) / (clock_end - clock_start);
        /* if (t_relative < 0) t_relative = 0; /* clamp t_relative - probably fine if it's negative */
        if (t_relative > 1) t_relative = 1;

        return ((frame_end - frame_start) * t_relative + frame_start) / sample_rate;
    }

    inline double reverse_map(double song_time, double sample_rate) {
        return double(song_time * sample_rate - frame_start) /
               double(frame_end - frame_start)
               * (clock_end - clock_start) + clock_start;
    }
};
//
//struct atomic_stream_time_t {
//    stream_time_map_t clock_map[2];
//    unsigned char clock_map_index;
//};

class AudioStream : public Sound
{
    class AudioStreamInternal;

    std::unique_ptr<AudioStreamInternal> internal;
    

    std::unique_ptr<AudioDataSource> mSource;
    unsigned int     mBufferSize;
    std::vector<short>	 mDecodedData;
    std::vector<short>	 mResampleBuffer;
    short			 tbuf[8192];
    double			 mStreamTime;
    double			 mPlaybackTime;

    /* total # of frames pulled after a Read operation. Can be negative for syncing purposes.
     * Is in the sampling rate of the target sample rate, not the source sample rate */
    int64_t           mReadFrames;

    bool			 mIsPlaying;
    IMixer *mOwnerMixer;
    stream_time_map_t current_clock;
public:
    AudioStream();
    AudioStream(IMixer* owner_mixer);
    ~AudioStream();

//    std::atomic<atomic_stream_time_t> dac_clock;

    uint32_t Read(float* buffer, size_t count) override;
    bool Open(std::filesystem::path Filename) override;
    void Play() override;
    void SeekTime(float Second) override;
    void SeekSample(uint32_t Sample) override;
    void Stop() override;
    bool IsValid();

    int64_t GetReadFrames() const;

    double GetStreamedTime() const;
    double GetPlayedTime() const;
    bool QueueStreamClock(const stream_time_map_t &map);

    /* maps a stream clock time to a point in time of the song. */
    double MapStreamClock(double stream_clock);


    uint32_t GetRate() const;

    uint32_t UpdateDecoder();
    bool IsPlaying() const override;
};
