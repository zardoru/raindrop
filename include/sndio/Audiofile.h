#pragma once


class AudioDataSource
{
protected:
    bool mSourceLoop;
public:
    AudioDataSource();
    virtual ~AudioDataSource();
    virtual bool open(std::filesystem::path Filename) = 0;
    virtual uint32_t read(short* buffer, size_t count) = 0; // count is in samples.
    virtual void seek(float Time) = 0;
    virtual size_t get_length() = 0; // Always returns total frames.
    virtual uint32_t get_rate() = 0; // Returns sampling rate of audio
    virtual uint32_t get_channels() = 0; // Returns channels of audio
    virtual bool is_valid() = 0;
    virtual bool has_data_left() = 0;

    void set_looping(bool loop);
};

class Sound
{
protected:
    uint32_t channels_ = 0;
    bool m_is_looping_ = false;
    double m_pitch_ = 0;
public:
    virtual ~Sound() = default;
    virtual uint32_t read(float* buffer, size_t count) = 0;
    virtual bool open(std::filesystem::path Filename) = 0;
    virtual void play() = 0;
    virtual bool is_playing() const = 0;
    virtual auto seek_time(float Second) -> void = 0;
    virtual void seek_sample(uint32_t Sample) = 0;
    virtual void stop() = 0;
    void set_pitch(double pitch);
    double get_pitch() const;
    void set_loop(bool loop);
    bool is_looping() const;
    uint32_t get_channels() const;
};

class IMixer;

class AudioSample : public Sound
{
    uint32_t	 m_rate_{};
    uint32_t   m_counter_{};
    float    m_audio_start_, m_audio_end_;
    std::shared_ptr<std::vector<short>> m_data_;
    bool	 m_is_playing_;
    std::atomic<bool> m_is_valid_;
	std::atomic<bool> m_is_loaded_;
	std::future<bool> m_thread_;
    IMixer *m_owner_mixer_;
public:
    AudioSample();
    AudioSample(IMixer* owner_mixer);
    AudioSample(const AudioSample& other);
    AudioSample(AudioSample &&other) noexcept;
    ~AudioSample();
	void seek(size_t offs);
	uint32_t read(float* buffer, size_t count) override;
    bool open(std::filesystem::path Filename) override;
    bool open(const std::filesystem::path &filename, bool async);
    bool open(AudioDataSource* source, bool async = false);
    void play() override;
    void seek_time(float Second) override;
    void seek_sample(uint32_t Sample) override;
    void stop() override;

	bool await_load() const;

	// returns duration in seconds
	double get_duration() const;

    bool is_playing() const override;
    void slice(float audio_start, float audio_end);
    std::shared_ptr<AudioSample> copy_slice();
    // void Mix(AudioSample& Other);
    bool is_valid() const;

    bool inner_load(AudioDataSource *src);
};

struct stream_time_map_t {
    double clock_start, clock_end;
    int64_t frame_start, frame_end;
    inline double map(double clock, double sample_rate) const {
        double t_relative = (clock - clock_start) / (clock_end - clock_start);
        if (t_relative > 1) t_relative = 1;

        return ((frame_end - frame_start) * t_relative + frame_start) / sample_rate;
    }

    inline double reverse_map(double song_time, double sample_rate) const {
        return (song_time * sample_rate - frame_start) /
               static_cast<double>(frame_end - frame_start)
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

    std::unique_ptr<AudioStreamInternal> internal_;
    

    std::unique_ptr<AudioDataSource> m_source_;
    unsigned int     m_buffer_size_{};
    std::vector<short>	 m_decoded_data_;
    std::vector<uint8_t>	 m_resample_buffer_;
    short			 audio_buffer_[8192]{};
    double			 m_stream_time_;
    double			 m_playback_time_{};

    /* total # of frames pulled after a Read operation. Can be negative for syncing purposes.
     * Is in the sampling rate of the target sample rate, not the source sample rate */
    int64_t          m_read_frames_;

    bool			 m_is_playing_;
    IMixer *m_owner_mixer_;
    stream_time_map_t current_clock_{};
public:
    AudioStream();
    explicit AudioStream(IMixer* owner_mixer);
    ~AudioStream() override;

//    std::atomic<atomic_stream_time_t> dac_clock;

    uint32_t read(float* buffer, size_t count) override;
    bool open(std::filesystem::path filename) override;
    void play() override;
    void seek_time(float second) override;
    void seek_sample(uint32_t Sample) override;
    void stop() override;
    bool is_valid() const;

    int64_t get_read_frames() const;

    double get_streamed_time() const;
    double get_played_time() const;
    bool queue_stream_clock(const stream_time_map_t &map) const;

    /* maps a stream clock time to a point in time of the song. */
    double map_stream_clock(double stream_clock);


    uint32_t get_rate() const;

    uint32_t update_decoder();
    bool is_playing() const override;
};
