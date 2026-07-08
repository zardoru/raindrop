#include <array>
#include <cstdio>
#include "av-io-common-pch.h"
#include "Audiofile.h"
#include "IMixer.h"
#include "AudioSourceSFM.h"
#include "AudioSourceOGG.h"

#include "pa_ringbuffer.h"

#include "AudioSourceMP3.h"

#include <text_and_file_util.h>
#include "rmath.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>

/* wraps around SDL_AudioStream using the settings we're likely to always use */
class SdlResampler {
    struct audio_stream_free_wrap {
        void operator()(SDL_AudioStream* p) const {
            SDL_DestroyAudioStream(p);
        }
    };

public:
    struct Config {
        double src_rate;
        double dst_rate;
        uint8_t input_channels;
        bool use_float;
    };

private:
    std::unique_ptr<SDL_AudioStream, audio_stream_free_wrap> m_resampler_;

    static SDL_AudioSpec get_input_spec(const Config& cfg) {
        return SDL_AudioSpec{
            .format = SDL_AUDIO_S16,
            .channels = cfg.input_channels,
            .freq = static_cast<int>(cfg.src_rate)
        };
    }

    static SDL_AudioSpec get_output_spec(const Config& cfg) {
        return SDL_AudioSpec{
            .format = cfg.use_float ? SDL_AUDIO_F32 : SDL_AUDIO_S16,
            .channels = 2,
            .freq = static_cast<int>(cfg.dst_rate)
        };
    }

public:
    Config last_config{};

    void Configure(const Config cfg)
    {
        if (cfg.input_channels != 1 && cfg.input_channels != 2)
            throw std::runtime_error("unexpected channel layout");

        const auto input_spec = get_input_spec(cfg);
        const auto output_spec = get_output_spec(cfg);
        m_resampler_.reset(SDL_CreateAudioStream(&input_spec, &output_spec));

        if (!m_resampler_)
            throw std::runtime_error(otoworm::util::format("error creating SDL audio stream: %s", SDL_GetError()));

        last_config = cfg;
    }

    void SetPitch(const double pitch)
    {
        if (!m_resampler_)
            return;

        if (!SDL_SetAudioStreamFrequencyRatio(m_resampler_.get(), static_cast<float>(pitch)))
            throw std::runtime_error(otoworm::util::format("error setting SDL resampler pitch: %s", SDL_GetError()));
    }

    void QueueInput(std::vector<uint8_t> &buffer_in) {
        if (!m_resampler_)
            return;

        if (!SDL_PutAudioStreamData(m_resampler_.get(), buffer_in.data(), static_cast<int>(buffer_in.size())))
            throw std::runtime_error(otoworm::util::format("error during resampling input: %s", SDL_GetError()));
    }

    int OutputBytesPerFrame() const {
        return (last_config.use_float ? sizeof(float) : sizeof(short)) * 2;
    }

    int AvailableOutputBytes() const {
        if (!m_resampler_)
            return 0;

        const int available = SDL_GetAudioStreamAvailable(m_resampler_.get());
        if (available < 0)
            throw std::runtime_error(otoworm::util::format("error checking SDL resampler output: %s", SDL_GetError()));

        return available;
    }

    /* returns: samples output per channel */
    int GetOutput(void *buffer_out, const size_t frames_out) {
        const int output_bytes_per_frame = OutputBytesPerFrame();
        const int bytes_requested = static_cast<int>(frames_out) * output_bytes_per_frame;
        const int bytes_read = SDL_GetAudioStreamData(m_resampler_.get(), buffer_out, bytes_requested);

        if (bytes_read < 0)
            throw std::runtime_error(otoworm::util::format("error during resampling output: %s", SDL_GetError()));

        return bytes_read / output_bytes_per_frame;
    }

    void Flush()
    {
        if (!m_resampler_)
            return;

        if (!SDL_FlushAudioStream(m_resampler_.get()))
            throw std::runtime_error(otoworm::util::format("error flushing resampler: %s", SDL_GetError()));
    }

    int Resample(std::vector<uint8_t> &buffer_in, void *buffer_out, const size_t frames_out) {
        QueueInput(buffer_in);
        return GetOutput(buffer_out, frames_out);
    }
};

class AudioStream::AudioStreamInternal
{
    public:
    PaUtilRingBuffer mDecodedDataRingbuffer{};

    std::array<uint8_t, 4096> mBufClockData{};
    PaUtilRingBuffer mBufClock{};
    SdlResampler mResampler;
};

template<class T>
void s16tof32(const T& iterable_start, const T& iterable_end, float* output)
{
    for (auto s = iterable_start; s != iterable_end; ++s)
    {
        if (*s < 0) *output = -float(*s) / std::numeric_limits<short>::min();
        else *output = float(*s) / std::numeric_limits<short>::max();
        output++;
    }
}

// Buffer -> buffer to convert to stereo (interleaved) cnt -> current samples max_len -> Maximum samples
template<class T>
void monoToStereo(T* Buffer, const size_t cnt, const size_t max_len)
{
    if (!cnt)
        return;

    if (cnt <= max_len / 2) // We're within boundaries.
    {
        // Okay, we have to transform all the samples from 0, 1, 2, 3...
        // to 0, 2, 4, 6... We start at the end so we don't have anything to overwrite.
        // We also need, after that is done, to copy all samples 0, 2, 4, 6 into 1, 3, 5, 7 and so on.
        // So interleave them...
        for (int i = cnt + 1; i >= 0; i--)
        {
            Buffer[i * 2 + 1] = Buffer[i];
            Buffer[i * 2] = Buffer[i];
        }
    }
}

std::unique_ptr<AudioDataSource> SourceFromExt(std::filesystem::path Filename)
{
    std::unique_ptr<AudioDataSource> Ret = nullptr;
    auto ext = otoworm::locale::wstring_to_utf8(Filename.extension().wstring());
	auto u8fn = otoworm::locale::wstring_to_utf8(Filename.wstring());
    
    if (u8fn.length() == 0 || ext.length() == 0)
    {
        // Log::Printf("Invalid filename. (%s) (%s)\n", u8fn.c_str(), u8fn.c_str());
        return nullptr;
    }

    otoworm::util::to_lower(ext);

    const char* xt = ext.c_str();
    if (strstr(xt, "wav") || strstr(xt, "flac"))
        Ret = std::make_unique<AudioSourceSFM>();
    else if (strstr(xt, "mp3") || strstr(xt, "ftb")) // az: ftb hax lol
        Ret = std::make_unique<AudioSourceMP3>();
    else if (strstr(xt, "ogg"))
        Ret = std::make_unique<AudioSourceOGG>();

    if (Ret && Ret->open(Filename))
		return Ret;
    else
    {
        // Log::Printf("extension %s has no audiosource associated\n", ext.c_str());
        return nullptr;
    }
}

void Sound::set_pitch(const double Pitch)
{
    m_pitch_ = Pitch;
}

double Sound::get_pitch() const
{
    return m_pitch_;
}

void Sound::set_loop(const bool Loop)
{
    m_is_looping_ = Loop;
}

bool Sound::is_looping() const
{
    return m_is_looping_;
}

uint32_t Sound::get_channels() const
{
    return channels_;
}

AudioSample::AudioSample()
{
    m_pitch_ = 1;
    m_is_playing_ = false;
    m_is_valid_ = false;
    m_is_looping_ = false;
    m_owner_mixer_ = nullptr;

    m_audio_start_ = 0;
    m_audio_end_ = std::numeric_limits<float>::infinity();
    // MixerAddSample(this);
}

AudioSample::AudioSample(IMixer* owner_mixer) : AudioSample()
{
    m_owner_mixer_ = owner_mixer;
    if (m_owner_mixer_)
        m_owner_mixer_->add_sample(this);
}

AudioSample::AudioSample(const AudioSample& Other)
{
    m_pitch_ = Other.m_pitch_;
    m_is_valid_ = (bool)Other.m_is_valid_;
    m_is_looping_ = Other.m_is_looping_;
	m_is_loaded_ = (bool)Other.m_is_loaded_;
    m_owner_mixer_ = Other.m_owner_mixer_;
    m_audio_start_ = Other.m_audio_start_;
    m_audio_end_ = Other.m_audio_end_;
    m_rate_ = Other.m_rate_;
    m_data_ = Other.m_data_;
    m_counter_ = 0;
    channels_ = Other.channels_;
    m_is_playing_ = false;
    if (m_owner_mixer_)
        m_owner_mixer_->add_sample(this);
}

AudioSample::AudioSample(AudioSample&& Other)
 noexcept {
    m_pitch_ = Other.m_pitch_;
    m_is_valid_ = (bool)Other.m_is_valid_;
    m_is_looping_ = Other.m_is_looping_;
    m_owner_mixer_ = Other.m_owner_mixer_;

	if (!Other.m_is_loaded_)
		Other.m_thread_.wait();

	m_is_loaded_ = true;

    m_audio_start_ = Other.m_audio_start_;
    m_audio_end_ = Other.m_audio_end_;
    m_rate_ = Other.m_rate_;
    m_data_ = Other.m_data_;
    m_counter_ = 0;
    channels_ = Other.channels_;
    m_is_playing_ = false;
    if (m_owner_mixer_)
        m_owner_mixer_->add_sample(this);
}

AudioSample::~AudioSample()
{
    if (m_owner_mixer_)
        m_owner_mixer_->remove_sample(this);
}

void AudioSample::seek(const size_t offs)
{
	m_counter_ = clamp(offs, (size_t)0, m_data_->size());
}


bool AudioSample::open(AudioDataSource* Src, const bool async)
{
    if (Src && Src->is_valid())
    {

		if (async)
            m_thread_ = std::async(std::launch::async, [&] { return inner_load(Src); });
		else
            inner_load(Src);


        return true;
    }
    return false;
}

bool AudioSample::inner_load(AudioDataSource *Src) {
    channels_ = Src->get_channels();
    size_t mSampleCount = Src->get_length() * channels_;

    if (!mSampleCount) // Huh what why?
        return false;

    m_data_ = std::make_shared<std::vector<short>>(mSampleCount);
    size_t total = Src->read(m_data_->data(), mSampleCount);

    if (total < mSampleCount) // Oh, odd. Oh well.
    {
        mSampleCount = total;
        m_data_->resize(mSampleCount); // trim trailing unread samples so resampler doesn't consume phantom frames
    }

    m_rate_ = Src->get_rate();

    double rate = m_rate_;
    if (m_owner_mixer_)
        rate = m_owner_mixer_->get_rate();

    if (m_rate_ != rate || m_pitch_ != 1)
    {
        double DstRate = rate;
        double ResamplingRate = DstRate / m_rate_;

        auto totalResampledSamples = size_t(ceil(mSampleCount * ResamplingRate));
        auto totalOutputFrameCount = totalResampledSamples / channels_ * 2; /* *2 because we want stereo output. */
        auto new_data = std::make_shared<std::vector<uint8_t>>(totalOutputFrameCount * sizeof (short));

        {
            SdlResampler::Config cfg = {
                    .src_rate = static_cast<double>(m_rate_),
                    .dst_rate = DstRate,
                    .input_channels = static_cast<uint8_t>(channels_),
                    .use_float = false
            };

            SdlResampler resampler;
            resampler.Configure(cfg);

            /* here's hoping the compiler is smart. */
            std::vector<uint8_t> vec_in(m_data_->size() * sizeof (short));
            memcpy(vec_in.data(), m_data_->data(), vec_in.size());

            auto &vec_out = *new_data;
            resampler.QueueInput(vec_in);
            resampler.Flush();
            auto size_out = resampler.GetOutput(vec_out.data(), totalResampledSamples / channels_);
            // size_out is frames-per-channel actually produced by swr; output is stereo (2 channels)
            new_data->resize(size_out * 2 * sizeof(short));
        }


        m_data_->resize(new_data->size() / sizeof (short));
        memcpy(m_data_->data(), new_data->data(), new_data->size());
        // resampled output is always stereo regardless of input channel count
        channels_ = 2;

        m_rate_ = rate;
    }

    m_counter_ = 0;
    m_is_valid_ = true;

    m_audio_end_ = (float(m_data_->size()) / (float(m_rate_) * channels_));
    m_is_loaded_ = true;

    return true;
}

uint32_t AudioSample::read(float* buffer, size_t count)
{
    size_t limit = (m_rate_ * channels_ * m_audio_end_);

    if (!m_is_playing_ || !m_is_loaded_)
        return 0;


_read:
    if (m_is_valid_ && count && !m_data_->empty())
    {
		size_t buffer_left = limit - m_counter_;
        uint32_t read_amount = std::min(buffer_left, count);

        if (m_counter_ < limit)
        {
            s16tof32(m_data_->begin() + m_counter_,
                     m_data_->begin() + m_counter_ + read_amount,
				     buffer);
            m_counter_ += read_amount;
			count -= read_amount;
        }

		if (m_counter_ == limit || count) {
			if (!m_is_looping_)
				m_is_playing_ = false;
			else
			{
				seek_time(m_audio_start_);
				buffer += read_amount;

				// note: implicit - count gets checked again
				// and it was already updated
				// basically call itself again. a goto is less expensive than a recursive call
				// and less likely to overflow (impossible, probably).
				// TODO: audio cracks a bit. not a big deal since
				// as of now (27-05-2016) I don't use loop with samples.
				goto _read;
			}
		}

        return read_amount;
    }
    else
        return 0;
}

double AudioSample::get_duration()
{
	return m_audio_end_ - m_audio_start_;
}

bool AudioSample::is_playing() const
{
    return m_is_playing_;
}

void AudioSample::slice(const float audio_start, const float audio_end)
{
	if (!m_is_loaded_) m_thread_.wait();

    float audioDuration = float(m_data_->size()) / (float(m_rate_) * channels_);
    m_audio_start_ = clamp(float(audio_start / m_pitch_), 0.0f, audioDuration);
    m_audio_end_ = clamp(float(audio_end / m_pitch_), m_audio_start_, audioDuration);
}

std::shared_ptr<AudioSample> AudioSample::CopySlice()
{
	if (!m_is_loaded_)
		m_thread_.wait();

    size_t start = clamp(size_t(m_audio_start_ * m_rate_ * channels_), size_t(0), m_data_->size());
    size_t end = clamp(size_t(m_audio_end_ * m_rate_ * channels_), start, m_data_->size());

    if (!m_audio_end_) throw std::runtime_error("No buffer available");
    if (end < start) throw std::runtime_error("warning copy slice: end < start");

    std::shared_ptr<AudioSample> out = std::make_shared<AudioSample>(*this);
    return out;
}

bool AudioSample::is_valid() const
{
    return m_data_ != nullptr && m_data_->size() != 0;
}

std::filesystem::path RearrangeFilename(std::filesystem::path Fn)
{
    std::filesystem::path Ret;
    if (std::filesystem::exists(Fn))
        return Fn;
    else
    {
        auto Ext = otoworm::locale::wstring_to_utf8(Fn.extension().wstring());
        otoworm::util::to_lower(Ext);

        if (Ext == ".wav")
            Ret = Fn.parent_path() / (Fn.stem().wstring() + L".ogg");
        else
            Ret = Fn.parent_path() / (Fn.stem().wstring() + L".wav");

        if (!std::filesystem::exists(Ret))
            return Fn;
        else
            return Ret;
    }
}

bool AudioSample::open(const std::filesystem::path Filename)
{
    auto FilenameFixed = RearrangeFilename(Filename);
    std::unique_ptr<AudioDataSource> Src = SourceFromExt(FilenameFixed);
    return open(Src.get());
}

bool AudioSample::open(std::filesystem::path Filename, const bool async)
{
	auto fn = [=]() {
		auto FilenameFixed = RearrangeFilename(Filename);
		std::unique_ptr<AudioDataSource> Src = SourceFromExt(FilenameFixed);
		return this->open(Src.get(), false);
	};

	if (async)
	{
		m_thread_ = std::async(std::launch::async, fn);
		return true;
	}
	else {
		return fn();
	}
}

void AudioSample::play()
{
    if (!is_valid()) return;

	if (!m_is_loaded_ && m_thread_.valid())
		m_thread_.wait();

    m_is_playing_ = true;
    seek_time(m_audio_start_);
}

void AudioSample::seek_time(const float Second)
{
    m_counter_ = m_rate_ * Second * channels_;

	if (!m_data_) return;

    if (m_counter_ >= m_data_->size())
        m_counter_ = m_data_->size();
}

void AudioSample::seek_sample(const uint32_t Sample)
{
    m_counter_ = Sample;

    if (m_counter_ >= m_data_->size())
        m_counter_ = m_data_->size();
}

void AudioSample::stop()
{
    m_is_playing_ = false;
}

bool AudioSample::await_load()
{
	if (!m_is_loaded_ && m_thread_.valid())
	{
		m_thread_.wait();
		return true;
	}

	return false;
}

AudioStream::AudioStream()
{
    m_pitch_ = 1;
    m_is_playing_ = false;
    m_is_looping_ = false;
    m_source_ = nullptr;
    m_owner_mixer_ = nullptr;
    internal_ = std::make_unique<AudioStreamInternal>();

    current_clock_ = {};

    // dac_clock.store({});

    m_read_frames_ = 0;
    m_stream_time_ = 0;

    // MixerAddStream(this);
}

AudioStream::~AudioStream()
{
    if (m_owner_mixer_)
        m_owner_mixer_->remove_stream(this);
}

uint32_t AudioStream::read(float* buffer, const size_t count)
{
    ring_buffer_size_t requested_samples_to_read = count; // Count is the amount of stereo output samples.
    size_t padded = 0;

    if (!m_source_ || !m_source_->is_valid())
    {
        m_is_playing_ = false;
        return 0;
    }

    if (m_is_playing_ && m_read_frames_ < 0) {
        /* advance buffer padding first */
        ring_buffer_size_t read_frames_positive = abs(m_read_frames_);

        /* multiply frames by channels (2) to get samples */
        int64_t padding_len = std::min(requested_samples_to_read, read_frames_positive * 2);
        memset(buffer, 0, padding_len * sizeof(float));
        buffer += padding_len;
        requested_samples_to_read -= padding_len;

        if (m_owner_mixer_) {
            auto rate_ratio = double (get_rate()) / double (m_owner_mixer_->get_rate());
            auto len = padding_len / 2 * rate_ratio;
            m_read_frames_ += len;
        } else
            m_read_frames_ += padding_len / 2;

        padded = padding_len;
    }

    if (m_is_playing_)
    {
        const size_t requested_output_frames = requested_samples_to_read / 2;
        const int bytes_requested = static_cast<int>(requested_output_frames) * internal_->mResampler.OutputBytesPerFrame();
        size_t decoded_input_frames = 0;

        internal_->mResampler.SetPitch(m_pitch_);

        while (internal_->mResampler.AvailableOutputBytes() < bytes_requested) {
            auto samples_to_read = PaUtil_GetRingBufferReadAvailable(&internal_->mDecodedDataRingbuffer);
            if (!samples_to_read)
                break;

            samples_to_read -= samples_to_read % channels_;
            if (!samples_to_read)
                break;

            m_resample_buffer_.resize(samples_to_read * sizeof (short));
            const size_t decoded_samples_read = PaUtil_ReadRingBuffer(
                    &internal_->mDecodedDataRingbuffer,
                    m_resample_buffer_.data(),
                    samples_to_read);
            if (!decoded_samples_read)
                break;

            m_resample_buffer_.resize(decoded_samples_read * sizeof(short));
            internal_->mResampler.QueueInput(m_resample_buffer_);
            decoded_input_frames += decoded_samples_read / channels_;
        }

        const size_t total_output_frames = internal_->mResampler.GetOutput(buffer, requested_output_frames);

        m_read_frames_ += total_output_frames;
        m_stream_time_ += static_cast<double>(decoded_input_frames) / m_source_->get_rate();
        return total_output_frames * 2 /* we output stereo */ + padded;
    }

    return 0;
}


bool AudioStream::open(const std::filesystem::path filename)
{
    m_source_ = SourceFromExt(RearrangeFilename(filename));

    if (m_source_)
    {
        channels_ = m_source_->get_channels();

        double dst_rate = m_source_->get_rate();
        if (m_owner_mixer_) dst_rate = m_owner_mixer_->get_rate();

        SdlResampler::Config cfg{};
        cfg.input_channels = channels_;
        cfg.dst_rate = dst_rate;
        cfg.src_rate = m_source_->get_rate();
        cfg.use_float = true;

        internal_->mResampler.Configure(cfg);

        m_buffer_size_ = BUFF_SIZE;
        m_decoded_data_.resize(m_buffer_size_);
        assert(m_decoded_data_.size() == m_buffer_size_);
        PaUtil_InitializeRingBuffer(
            &internal_->mDecodedDataRingbuffer,
            sizeof(short), 
            m_buffer_size_,
            m_decoded_data_.data()
        );

        PaUtil_InitializeRingBuffer(
            &internal_->mBufClock,
            sizeof(stream_time_map_t),
            4096 / sizeof(stream_time_map_t),
            internal_->mBufClockData.data()
        );

        current_clock_ = {};

        m_stream_time_ = m_playback_time_ = m_read_frames_ = 0;

        seek_time(0);

        return true;
    }

    return false;
}

bool AudioStream::is_playing() const
{
    return m_is_playing_;
}

void AudioStream::play()
{
	if (m_source_ && m_source_->is_valid()) {
		m_is_playing_ = true;
	}
}

void AudioStream::seek_time(const float second)
{
    if (m_source_) {
        m_source_->seek(second);
    }

    m_read_frames_ = second * get_rate();
    m_stream_time_ = second;
}

double AudioStream::get_streamed_time() const
{
    return m_stream_time_;
}

double AudioStream::get_played_time() const
{
    return m_playback_time_;
}

void AudioStream::seek_sample(const uint32_t Sample)
{
    m_source_->seek(static_cast<float>(Sample) / m_source_->get_rate());
}

void AudioStream::stop()
{
    m_is_playing_ = false;
}

uint32_t AudioStream::update_decoder()
{
    const uint32_t avail_write_count = PaUtil_GetRingBufferWriteAvailable(&internal_->mDecodedDataRingbuffer);
    uint32_t read_total;

    if (!m_source_ || !m_source_->is_valid()) return 0;

    m_source_->set_looping(is_looping());

    if ((read_total = m_source_->read(audio_buffer_, avail_write_count)))
    {
        PaUtil_WriteRingBuffer(&internal_->mDecodedDataRingbuffer, audio_buffer_, read_total);
    }
    else
    {
        if (!PaUtil_GetRingBufferReadAvailable(&internal_->mDecodedDataRingbuffer) && !m_source_->has_data_left())
            m_is_playing_ = false;
    }

    return read_total;
}

uint32_t AudioStream::get_rate() const
{
    if (m_owner_mixer_)
        return m_owner_mixer_->get_rate();

    return m_source_->get_rate();
}

double AudioStream::map_stream_clock(const double stream_clock) {
    if (m_read_frames_ == 0) return 0; /* no data has been streamed */

    while (current_clock_.clock_end < stream_clock) { /* this is over */

        /* record start time */
        if (current_clock_.frame_start <= 0 &&
            current_clock_.frame_end >= 0 &&
            current_clock_.clock_start != 0) {
            m_playback_time_ = current_clock_.reverse_map(0, get_rate());
        }

        /* if there are no pending clock maps on the ring buffer */
        if (!PaUtil_ReadRingBuffer(&internal_->mBufClock, &current_clock_, 1)) {
            break;
        }
    }

    return current_clock_.map(stream_clock, get_rate());
}

int64_t AudioStream::get_read_frames() const {
    return m_read_frames_;
}

bool AudioStream::queue_stream_clock(const stream_time_map_t& map) const
{
    if (PaUtil_GetRingBufferWriteAvailable(&internal_->mBufClock) > 0) {
        PaUtil_WriteRingBuffer(&internal_->mBufClock, &map, 1);
        return true;
    }

    return false;
}


AudioDataSource::AudioDataSource()
{
	mSourceLoop = false;
}

AudioDataSource::~AudioDataSource()
{
}

void AudioDataSource::set_looping(const bool Loop)
{
    mSourceLoop = Loop;
}

bool AudioStream::is_valid() const
{
    return m_source_ && m_source_->is_valid();
}

AudioStream::AudioStream(IMixer *owner_mixer) : AudioStream() {
    m_owner_mixer_ = owner_mixer;
    m_owner_mixer_->add_stream(this);
}
