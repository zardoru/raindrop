#include <array>
#include "av-io-common-pch.h"
#include "Audiofile.h"
#include "IMixer.h"
#include "AudioSourceSFM.h"
#include "AudioSourceOGG.h"

#include "pa_ringbuffer.h"

#include "AudioSourceMP3.h"

#include "TextAndFileUtil.h"
#include "rmath.h"

extern "C" {
#include <libswresample/swresample.h>
}

/* wraps around libSwResample using the settings we're likely to Always Use */
class SwrResampler {
    struct swr_free_wrap {
        void operator()(SwrContext* p) const {
            swr_free(&p);
        }
    };

    std::unique_ptr<SwrContext, swr_free_wrap>			 mResampler;
public:
    struct Config {
        double src_rate;
        double dst_rate;
        uint8_t input_channels;
        bool use_float;

        AVSampleFormat get_output_sample_format() const {
            if (use_float)
                return AV_SAMPLE_FMT_FLT;
            else
                return AV_SAMPLE_FMT_S16;
        }

        int64_t get_input_channel_layout() const {
            if (input_channels == 2)
                return AV_CH_LAYOUT_STEREO;
            else if (input_channels == 1)
                return AV_CH_LAYOUT_MONO;
            else {
                throw std::runtime_error("unexpected channel layout");
            }
        }
    };

    Config last_config{};

    SwrResampler () : mResampler(swr_alloc(), swr_free_wrap()) {}

    void Configure(const Config cfg) {
        swr_alloc_set_opts(
                mResampler.get(),
                AV_CH_LAYOUT_STEREO,
                cfg.get_output_sample_format(),
                cfg.dst_rate,
                cfg.get_input_channel_layout(),
                AV_SAMPLE_FMT_S16,
                cfg.src_rate,
                0,
                NULL
        );

        swr_init(mResampler.get());
        last_config = cfg;
    }

    /* returns: samples output per channel */
    int Resample(std::vector<uint8_t> &buffer_in, void *buffer_out, size_t frames_out) {
        auto frames_in = buffer_in.size() / sizeof (short) / last_config.input_channels;

        const uint8_t * in_ptr = buffer_in.data();

        auto res = swr_convert(mResampler.get(), reinterpret_cast<uint8_t **>(&buffer_out), frames_out, &in_ptr, frames_in);
        if (res < 0) {
            // error? mm
            throw std::runtime_error(Utility::Format("error during resampling %d", res));
        }

        return res;
    }
};

class AudioStream::AudioStreamInternal
{
    public:
    PaUtilRingBuffer mDecodedDataRingbuffer{};

    std::array<uint8_t, 4096> mBufClockData{};
    PaUtilRingBuffer mBufClock{};
    SwrResampler mResampler;
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
void monoToStereo(T* Buffer, size_t cnt, size_t max_len)
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
    auto ext = Conversion::ToU8(Filename.extension().wstring());
	auto u8fn = Conversion::ToU8(Filename.wstring());
    
    if (u8fn.length() == 0 || ext.length() == 0)
    {
        // Log::Printf("Invalid filename. (%s) (%s)\n", u8fn.c_str(), u8fn.c_str());
        return nullptr;
    }

    Utility::ToLower(ext);

    const char* xt = ext.c_str();
    if (strstr(xt, "wav") || strstr(xt, "flac"))
        Ret = std::make_unique<AudioSourceSFM>();
    else if (strstr(xt, "mp3") || strstr(xt, "ftb")) // az: ftb hax lol
        Ret = std::make_unique<AudioSourceMP3>();
    else if (strstr(xt, "ogg"))
        Ret = std::make_unique<AudioSourceOGG>();

    if (Ret && Ret->Open(Filename))
		return Ret;
    else
    {
        // Log::Printf("extension %s has no audiosource associated\n", ext.c_str());
        return nullptr;
    }
}

void Sound::SetPitch(double Pitch)
{
    mPitch = Pitch;
}

double Sound::GetPitch() const
{
    return mPitch;
}

void Sound::SetLoop(bool Loop)
{
    mIsLooping = Loop;
}

bool Sound::IsLooping() const
{
    return mIsLooping;
}

uint32_t Sound::GetChannels() const
{
    return Channels;
}

AudioSample::AudioSample()
{
    mPitch = 1;
    mIsPlaying = false;
    mIsValid = false;
    mIsLooping = false;
    mOwnerMixer = nullptr;

    mAudioStart = 0;
    mAudioEnd = std::numeric_limits<float>::infinity();
    // MixerAddSample(this);
}

AudioSample::AudioSample(IMixer* owner_mixer) : AudioSample()
{
    mOwnerMixer = owner_mixer;
    if (mOwnerMixer)
        mOwnerMixer->AddSample(this);
}

AudioSample::AudioSample(const AudioSample& Other)
{
    mPitch = Other.mPitch;
    mIsValid = (bool)Other.mIsValid;
    mIsLooping = Other.mIsLooping;
	mIsLoaded = (bool)Other.mIsLoaded;
    mOwnerMixer = Other.mOwnerMixer;
    mAudioStart = Other.mAudioStart;
    mAudioEnd = Other.mAudioEnd;
    mRate = Other.mRate;
    mData = Other.mData;
    mCounter = 0;
    Channels = Other.Channels;
    mIsPlaying = false;
    if (mOwnerMixer)
        mOwnerMixer->AddSample(this);
}

AudioSample::AudioSample(AudioSample&& Other)
 noexcept {
    mPitch = Other.mPitch;
    mIsValid = (bool)Other.mIsValid;
    mIsLooping = Other.mIsLooping;
    mOwnerMixer = Other.mOwnerMixer;

	if (!Other.mIsLoaded)
		Other.mThread.wait();

	mIsLoaded = true;

    mAudioStart = Other.mAudioStart;
    mAudioEnd = Other.mAudioEnd;
    mRate = Other.mRate;
    mData = Other.mData;
    mCounter = 0;
    Channels = Other.Channels;
    mIsPlaying = false;
    if (mOwnerMixer)
        mOwnerMixer->AddSample(this);
}

AudioSample::~AudioSample()
{
    if (mOwnerMixer)
        mOwnerMixer->RemoveSample(this);
}

void AudioSample::Seek(size_t offs)
{
	mCounter = Clamp(offs, (size_t)0, mData->size());
}


bool AudioSample::Open(AudioDataSource* Src, bool async)
{
    if (Src && Src->IsValid())
    {

		if (async)
            mThread = std::async(std::launch::async, [&] { return InnerLoad(Src); });
		else
            InnerLoad(Src);


        return true;
    }
    return false;
}

bool AudioSample::InnerLoad(AudioDataSource *Src) {
    Channels = Src->GetChannels();
    size_t mSampleCount = Src->GetLength() * Channels;

    if (!mSampleCount) // Huh what why?
        return false;

    mData = std::make_shared<std::vector<short>>(mSampleCount);
    size_t total = Src->Read(mData->data(), mSampleCount);

    if (total < mSampleCount) // Oh, odd. Oh well.
        mSampleCount = total;

    mRate = Src->GetRate();

    double rate = mRate;
    if (mOwnerMixer)
        rate = mOwnerMixer->GetRate();

    if (mRate != rate || mPitch != 1)
    {
        size_t idone = 0;
        size_t odone = 0;
        double DstRate = rate / mPitch;
        double ResamplingRate = DstRate / mRate;

        auto totalResampledSamples = size_t(ceil(mSampleCount * ResamplingRate));
        auto totalOutputFrameCount = totalResampledSamples / Channels * 2; /* *2 because we want stereo output. */
        auto new_data = std::make_shared<std::vector<uint8_t>>(totalOutputFrameCount * sizeof (short));

        {
            SwrResampler::Config cfg = {
                    .src_rate = static_cast<double>(mRate),
                    .dst_rate = DstRate,
                    .input_channels = static_cast<uint8_t>(Channels),
                    .use_float = false
            };

            SwrResampler resampler;
            resampler.Configure(cfg);

            /* here's hoping the compiler is smart. */
            std::vector<uint8_t> vec_in(mData->size() * sizeof (short));
            memcpy(vec_in.data(), mData->data(), vec_in.size());

            auto &vec_out = *new_data;
            auto size_out = resampler.Resample(vec_in, vec_out.data(), totalResampledSamples / Channels);
            // Utility::DebugBreak();
        }


        mData->resize(new_data->size() / sizeof (short));
        memcpy(mData->data(), new_data->data(), new_data->size());

        mRate = rate;
    }

    mCounter = 0;
    mIsValid = true;

    mAudioEnd = (float(mData->size()) / (float(mRate) * Channels));
    mIsLoaded = true;

    return true;
}

uint32_t AudioSample::Read(float* buffer, size_t count)
{
    size_t limit = (mRate * Channels * mAudioEnd);

    if (!mIsPlaying || !mIsLoaded)
        return 0;


_read:
    if (mIsValid && count && !mData->empty())
    {
		size_t buffer_left = limit - mCounter;
        uint32_t read_amount = std::min(buffer_left, count);

        if (mCounter < limit)
        {
            s16tof32(mData->begin() + mCounter,
                     mData->begin() + mCounter + read_amount,
				     buffer);
            mCounter += read_amount;
			count -= read_amount;
        }

		if (mCounter == limit || count) {
			if (!mIsLooping)
				mIsPlaying = false;
			else
			{
				SeekTime(mAudioStart);
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

double AudioSample::GetDuration()
{
	return mAudioEnd - mAudioStart;
}

bool AudioSample::IsPlaying() const
{
    return mIsPlaying;
}

void AudioSample::Slice(float audio_start, float audio_end)
{
	if (!mIsLoaded) mThread.wait();

    float audioDuration = float(mData->size()) / (float(mRate) * Channels);
    mAudioStart = Clamp(float(audio_start / mPitch), 0.0f, audioDuration);
    mAudioEnd = Clamp(float(audio_end / mPitch), mAudioStart, audioDuration);
}

std::shared_ptr<AudioSample> AudioSample::CopySlice()
{
	if (!mIsLoaded)
		mThread.wait();

    size_t start = Clamp(size_t(mAudioStart * mRate * Channels), size_t(0), mData->size());
    size_t end = Clamp(size_t(mAudioEnd * mRate * Channels), start, mData->size());

    if (!mAudioEnd) throw std::runtime_error("No buffer available");
    if (end < start) throw std::runtime_error("warning copy slice: end < start");

    std::shared_ptr<AudioSample> out = std::make_shared<AudioSample>(*this);
    return out;
}

bool AudioSample::IsValid() const
{
    return mData != nullptr && mData->size() != 0;
}

std::filesystem::path RearrangeFilename(std::filesystem::path Fn)
{
    std::filesystem::path Ret;
    if (std::filesystem::exists(Fn))
        return Fn;
    else
    {
        auto Ext = Conversion::ToU8(Fn.extension().wstring());
        Utility::ToLower(Ext);

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

bool AudioSample::Open(std::filesystem::path Filename)
{
    auto FilenameFixed = RearrangeFilename(Filename);
    std::unique_ptr<AudioDataSource> Src = SourceFromExt(FilenameFixed);
    return Open(Src.get());
}

bool AudioSample::Open(std::filesystem::path Filename, bool async)
{
	auto fn = [=]() {
		auto FilenameFixed = RearrangeFilename(Filename);
		std::unique_ptr<AudioDataSource> Src = SourceFromExt(FilenameFixed);
		return this->Open(Src.get(), false);
	};

	if (async)
	{
		mThread = std::async(std::launch::async, fn);
		return true;
	}
	else {
		return fn();
	}
}

void AudioSample::Play()
{
    if (!IsValid()) return;

	if (!mIsLoaded && mThread.valid())
		mThread.wait();

    mIsPlaying = true;
    SeekTime(mAudioStart);
}

void AudioSample::SeekTime(float Second)
{
    mCounter = mRate * Second * Channels;

	if (!mData) return;

    if (mCounter >= mData->size())
        mCounter = mData->size();
}

void AudioSample::SeekSample(uint32_t Sample)
{
    mCounter = Sample;

    if (mCounter >= mData->size())
        mCounter = mData->size();
}

void AudioSample::Stop()
{
    mIsPlaying = false;
}

bool AudioSample::AwaitLoad()
{
	if (!mIsLoaded && mThread.valid())
	{
		mThread.wait();
		return true;
	}

	return false;
}

AudioStream::AudioStream()
{
    mPitch = 1;
    mIsPlaying = false;
    mIsLooping = false;
    mSource = nullptr;
    mOwnerMixer = nullptr;
    internal = std::make_unique<AudioStreamInternal>();

    current_clock = {};

    // dac_clock.store({});

    mReadFrames = 0;
    mStreamTime = 0;

    // MixerAddStream(this);
}

AudioStream::~AudioStream()
{
    if (mOwnerMixer)
        mOwnerMixer->RemoveStream(this);
}

uint32_t AudioStream::Read(float* buffer, size_t count)
{
    ring_buffer_size_t requested_samples_to_read = count; // Count is the amount of samples.
    size_t padded = 0;

    if (!mSource || !mSource->IsValid())
    {
        mIsPlaying = false;
        return 0;
    }

    if (mIsPlaying && mReadFrames < 0) {
        /* advance buffer padding first */
        ring_buffer_size_t read_frames_positive = abs(mReadFrames);

        /* multiply frames by channels (2) to get samples */
        int64_t padding_len = std::min(requested_samples_to_read, read_frames_positive * 2);
        memset(buffer, 0, padding_len * sizeof(float));
        buffer += padding_len;
        requested_samples_to_read -= padding_len;

        if (mOwnerMixer) {
            auto rate_ratio = double (GetRate()) / double (mOwnerMixer->GetRate());
            auto len = padding_len / 2 * rate_ratio;
            mReadFrames += len;
        } else
            mReadFrames += padding_len / 2;

        padded = padding_len;
    }

    if (Channels == 1) // We just want half the samples.
        requested_samples_to_read >>= 1;

    if (PaUtil_GetRingBufferReadAvailable(&internal->mDecodedDataRingbuffer) < requested_samples_to_read || !mIsPlaying)
        requested_samples_to_read = PaUtil_GetRingBufferReadAvailable(&internal->mDecodedDataRingbuffer);

    double dstrate = mSource->GetRate();
    if (mOwnerMixer)
        dstrate = mOwnerMixer->GetRate();

    if (mIsPlaying)
    {
        // This is what our destination rate will be
        double origRate = mSource->GetRate();

        // This is what our destination rate is.
        double resRate = dstrate / mPitch;
        double RateRatio = resRate / origRate;

        // This is how many samples we want to read from the source buffer
        size_t samples_to_read = ceil(origRate / resRate * requested_samples_to_read);

        if (samples_to_read & 1 && Channels == 2 && mPitch < 1) samples_to_read += 1; // make even (channels)
        else if (samples_to_read & 1 && Channels == 2 && mPitch > 1) samples_to_read -= 1; // also make even

        mResampleBuffer.resize(samples_to_read * sizeof (short));
        size_t decoded_samples_read = PaUtil_ReadRingBuffer(&internal->mDecodedDataRingbuffer, mResampleBuffer.data(), samples_to_read);
        // decoded_samples_read now contains how many samples we actually read...

        if (!decoded_samples_read)
            return padded; // case 1: pure padding. case 2: really just not enough data has been decoded

        size_t samples_to_output = round(decoded_samples_read * RateRatio);
        if (samples_to_output & 1 && samples_to_output < count) samples_to_output += 1; // make even (edge case... :S)

        // The count that soxr asks for I think, is frames, not samples. Thus, the division by channels.
        size_t total_output_frames;
        total_output_frames = internal->mResampler.Resample(mResampleBuffer, buffer, samples_to_output / 2);

        mReadFrames += total_output_frames;
        mStreamTime += double(total_output_frames) / mSource->GetRate();
        return total_output_frames * 2 /* we output stereo */ + padded;
    }

    return 0;
}


bool AudioStream::Open(std::filesystem::path Filename)
{
    mSource = SourceFromExt(RearrangeFilename(Filename));

    if (mSource)
    {
        Channels = mSource->GetChannels();

        double dst_rate = mSource->GetRate();
        if (mOwnerMixer) dst_rate = mOwnerMixer->GetRate();

        SwrResampler::Config cfg{};
        cfg.input_channels = Channels;
        cfg.dst_rate = dst_rate;
        cfg.src_rate = mSource->GetRate();
        cfg.use_float = true;

        internal->mResampler.Configure(cfg);

        mBufferSize = BUFF_SIZE;
        mDecodedData.resize(mBufferSize);
        assert(mDecodedData.size() == mBufferSize);
        PaUtil_InitializeRingBuffer(
            &internal->mDecodedDataRingbuffer, 
            sizeof(short), 
            mBufferSize, 
            mDecodedData.data()
        );

        PaUtil_InitializeRingBuffer(
            &internal->mBufClock,
            sizeof(stream_time_map_t),
            4096 / sizeof(stream_time_map_t),
            internal->mBufClockData.data()
        );

        current_clock = {};

        mStreamTime = mPlaybackTime = mReadFrames = 0;

        SeekTime(0);

        return true;
    }

    return false;
}

bool AudioStream::IsPlaying() const
{
    return mIsPlaying;
}

void AudioStream::Play()
{
	if (mSource && mSource->IsValid()) {
		mIsPlaying = true;
	}
}

void AudioStream::SeekTime(float Second)
{
    if (mSource) {
        mSource->Seek(Second);
    }

    mReadFrames = Second * GetRate();
    mStreamTime = Second;
}

double AudioStream::GetStreamedTime() const
{
    return mStreamTime;
}

double AudioStream::GetPlayedTime() const
{
    return mPlaybackTime;
}

void AudioStream::SeekSample(uint32_t Sample)
{
    mSource->Seek(float(Sample) / mSource->GetRate());
}

void AudioStream::Stop()
{
    mIsPlaying = false;
}

uint32_t AudioStream::UpdateDecoder()
{
    uint32_t eCount = PaUtil_GetRingBufferWriteAvailable(&internal->mDecodedDataRingbuffer);
    uint32_t ReadTotal;

    if (!mSource || !mSource->IsValid()) return 0;

    mSource->SetLooping(IsLooping());

    if ((ReadTotal = mSource->Read(tbuf, eCount)))
    {
        PaUtil_WriteRingBuffer(&internal->mDecodedDataRingbuffer, tbuf, ReadTotal);
    }
    else
    {
        if (!PaUtil_GetRingBufferReadAvailable(&internal->mDecodedDataRingbuffer) && !mSource->HasDataLeft())
            mIsPlaying = false;
    }

    return ReadTotal;
}

uint32_t AudioStream::GetRate() const
{
    return mSource->GetRate();
}

double AudioStream::MapStreamClock(double stream_clock) {
    if (mReadFrames == 0) return 0; /* no data has been streamed */

    while (current_clock.clock_end < stream_clock) { /* this is over */

        /* record start time */
        if (current_clock.frame_start <= 0 &&
            current_clock.frame_end >= 0 &&
            current_clock.clock_start != 0) {
            mPlaybackTime = current_clock.reverse_map(0, GetRate());
        }

        /* if there are no pending clock maps on the ring buffer */
        if (!PaUtil_ReadRingBuffer(&internal->mBufClock, &current_clock, 1)) {
            break;
        }
    }

    return current_clock.map(stream_clock, GetRate());
}

int64_t AudioStream::GetReadFrames() const {
    return mReadFrames;
}

bool AudioStream::QueueStreamClock(const stream_time_map_t& map) {
    if (PaUtil_GetRingBufferWriteAvailable(&internal->mBufClock) > 0) {
        PaUtil_WriteRingBuffer(&internal->mBufClock, &map, 1);
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

void AudioDataSource::SetLooping(bool Loop)
{
    mSourceLoop = Loop;
}

bool AudioStream::IsValid()
{
    return mSource && mSource->IsValid();
}

AudioStream::AudioStream(IMixer *owner_mixer) : AudioStream() {
    mOwnerMixer = owner_mixer;
    mOwnerMixer->AddStream(this);
}
