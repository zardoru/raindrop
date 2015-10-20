
#include <sndfile.h>
#include <soxr.h>

#include "Global.h"
#include "Audio.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"
#include "AudioSourceOGG.h"

#ifdef MP3_ENABLED
#include "AudioSourceMP3.h"
#endif

// Buffer -> buffer to convert to stereo (interleaved) cnt -> current samples max_len -> Maximum samples
void monoToStereo(float* Buffer, size_t cnt, size_t max_len)
{
	if (!cnt)
		return;

	if (cnt <= max_len/2) // We're within boundaries.
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

unique_ptr<AudioDataSource> SourceFromExt(Directory Filename)
{
	AudioDataSource *Ret = nullptr;
	GString Ext = Filename.GetExtension();
	Filename.Normalize();

	if (Filename.path().length() == 0 || Ext.length() == 0) {
		wprintf(L"Invalid filename. (%s) (%s)\n", Utility::Widen(Filename).c_str(), Utility::Widen(Ext).c_str());
		return nullptr;
	}

	Utility::ToLower(Ext);

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

	return unique_ptr<AudioDataSource>(Ret);
}

void Sound::SetPitch(double Pitch)
{
	mPitch = Pitch;
}

double Sound::GetPitch()
{
	return mPitch;
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
	mPitch = 1;
	mIsPlaying = false;
	mIsValid = false;
	mIsLooping = false;

	mAudioStart = 0;
	mAudioEnd = std::numeric_limits<float>::infinity();
	MixerAddSample(this);
}

AudioSample::AudioSample(AudioSample& Other)
{
	mPitch = Other.mPitch;
	mIsValid = Other.mIsValid;
	mIsLooping = Other.mIsLooping;

	mAudioStart = Other.mAudioStart;
	mAudioEnd = Other.mAudioEnd;
	mRate = Other.mRate;
	mData = Other.mData;
	mCounter = 0;
	Channels = Other.Channels;
	mByteSize = Other.mByteSize;
	mIsPlaying = false;
	MixerAddSample(this);
}

AudioSample::AudioSample(AudioSample&& Other)
{
	mPitch = Other.mPitch;
	mIsValid = Other.mIsValid;
	mIsLooping = Other.mIsLooping;

	mAudioStart = Other.mAudioStart;
	mAudioEnd = Other.mAudioEnd;
	mRate = Other.mRate;
	mData = Other.mData;
	mCounter = 0;
	Channels = Other.Channels;
	mByteSize = Other.mByteSize;
	mIsPlaying = false;
	MixerAddSample(this);
}

AudioSample::~AudioSample()
{
	MixerRemoveSample(this);
}

bool AudioSample::Open(AudioDataSource* Src)
{
	if (Src && Src->IsValid())
	{
		Channels = Src->GetChannels();
		size_t mSampleCount = Src->GetLength() * Channels;

		if (!mSampleCount) // Huh what why?
			return false;

		mData = make_shared<vector<float>> (mSampleCount);
		size_t total = Src->Read(mData->data(), mSampleCount);

		if (total < mSampleCount) // Oh, odd. Oh well.
			mSampleCount = total;

		mRate = Src->GetRate();

		if (Channels == 1) // Mono? We'll need to duplicate information for both channels.
		{
			size_t size = mSampleCount * 2;
			auto mDataNew = make_shared<vector<float>> (size);

			for (size_t i = 0, j = 0; i < mSampleCount; i++, j += 2)
			{
				(*mDataNew)[j] = (*mData)[i];
				(*mDataNew)[j + 1] = (*mData)[i];
			}

			mSampleCount = size;
			mData = mDataNew;
			Channels = 2;
		}

		if (mRate != 44100 || mPitch != 1) // mixer_constant.. in the future, allow changing this destination sample rate.
		{
			size_t done;
			size_t doneb;
			double DstRate = 44100.0 / mPitch;
			double ResamplingRate = DstRate / mRate;
			soxr_io_spec_t spc;
			size_t size = size_t(ceil(mSampleCount * ResamplingRate));
			shared_ptr<vector<float>> mDataNew = make_shared<vector<float>> (size);

			spc.e = nullptr;
			spc.itype = SOXR_FLOAT32_I;
			spc.otype = SOXR_FLOAT32_I;
			spc.scale = 1;
			spc.flags = 0;
			soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_VHQ, SOXR_VR);

			soxr_oneshot(mRate, DstRate, Channels,
				mData->data(), mSampleCount / Channels, &done,
				mDataNew->data(), size / Channels, &doneb,
				&spc, &q_spec, nullptr);

			mSampleCount = size;
			mData = mDataNew;
			mRate = 44100;
		}

		mCounter = 0;
		mIsValid = true;
		mByteSize = mSampleCount * sizeof(short);

		mAudioEnd = (float(mSampleCount) / (float(mRate) * Channels));

		return true;
	}
	return false;
}

uint32 AudioSample::Read(float* buffer, size_t count)
{
	if (!mIsPlaying)
		return 0;

	if (mIsValid)
	{
		size_t limit = (mRate * Channels * mAudioEnd);
		size_t bufferLeft = limit - mCounter;
		uint32 ReadAmount = min(bufferLeft, count);

		if (mCounter < limit)
		{
			memcpy(buffer, &(*mData)[0]+mCounter, ReadAmount * sizeof(float));
			mCounter += ReadAmount;
		}else
		{
			mIsPlaying = false;
			// memset(buffer, 0, count * sizeof(float));
		}

		return ReadAmount;
	}else
		return 0;

}

bool AudioSample::IsPlaying()
{
	return mIsPlaying;
}

void AudioSample::Slice(float audio_start, float audio_end)
{
	float audioDuration = float(mData->size()) / (float(mRate) * Channels);
	mAudioStart = Clamp(audio_start, 0.0f, audioDuration);
	mAudioEnd = Clamp(audio_end, mAudioStart, audioDuration);
}

shared_ptr<AudioSample> AudioSample::CopySlice()
{
	size_t start = Clamp(size_t(mAudioStart * mRate * Channels), size_t(0), mData->size());
	size_t end = Clamp(size_t(mAudioEnd * mRate * Channels), start, mData->size());

	if (!mAudioEnd) throw std::runtime_error("No buffer available");
	if (end < start) throw std::runtime_error("warning copy slice: end < start");

	shared_ptr<AudioSample> out = make_shared<AudioSample>(*this);
	return out;
}

void AudioSample::Mix(AudioSample& Other)
{
	size_t start = Clamp(size_t(mAudioStart * mRate * Channels), size_t(0), mData->size());
	size_t end = Clamp(size_t(mAudioEnd * mRate * Channels), start, mData->size());
	size_t startB = Clamp(size_t(Other.mAudioStart * Other.mRate * Other.Channels), size_t(0), Other.mData->size());
	size_t endB = Clamp(size_t(Other.mAudioEnd * Other.mRate * Other.Channels), startB, Other.mData->size());
	auto buf = make_shared<vector<float>>(max(endB - startB, end - start));

	for (size_t i = 0; i < buf->size(); i++)
	{
		size_t ai = start + i;
		size_t bi = startB + i;
		if (bi < Other.mData->size() && ai < mData->size())
			(*buf)[i] = (*Other.mData)[bi] + (*mData)[ai];
		if (bi >= Other.mData->size() && ai < mData->size())
			(*buf)[i] = (*mData)[ai];
		if (bi < Other.mData->size() && ai >= mData->size())
			(*buf)[i] = (*Other.mData)[bi];
	}

	mAudioStart = 0;
	mAudioEnd = float(buf->size()) / (mRate * Channels);
	mData = buf;
}

bool AudioSample::IsValid()
{
	return mData != nullptr && mData->size() != 0;
}

GString RearrangeFilename(const char* Fn)
{
	GString Ret;
	Directory File = Fn;
	File.Normalize();
	if (Utility::FileExists(File.c_path()))
		return Fn;
	else
	{
		GString Ext = File.GetExtension();
		Utility::ToLower(Ext);

		if (strstr(Ext.c_str(), "wav"))
			Ret = Utility::RemoveExtension(Fn) + ".ogg";
		else
			Ret = Utility::RemoveExtension(Fn) + ".wav";

		if (!Utility::FileExists(Ret))
			return File.c_path();
		else
			return Ret;
	}
}

bool AudioSample::Open(const char* Filename)
{
	GString FilenameFixed = RearrangeFilename(Filename);
	unique_ptr<AudioDataSource> Src = SourceFromExt (FilenameFixed);
	return Open(Src.get());
}

void AudioSample::Play()
{
	mIsPlaying = true;
	SeekTime(mAudioStart);
}

void AudioSample::SeekTime(float Second)
{
	mCounter = mRate * Second * Channels;

	if (mCounter >= mData->size())
		mCounter = mData->size();
}

void AudioSample::SeekSample(uint32 Sample)
{
	mCounter = Sample;

	if (mCounter >= mData->size())
		mCounter = mData->size();
}

void AudioSample::Stop()
{
	mIsPlaying = false;
}

AudioStream::AudioStream()
{
	mPitch = 1;
	mIsPlaying = false;
	mIsLooping = false;
	mSource = nullptr;
	mResampler = nullptr;

	MixerAddStream(this);
}

AudioStream::~AudioStream()
{
	MixerRemoveStream(this);

	soxr_delete(mResampler);
}

uint32 AudioStream::Read(float* buffer, size_t count)
{
	size_t cnt;
	ring_buffer_size_t toRead = count; // Count is the amount of samples.
	size_t outcnt;

	if (!mSource || !mSource->IsValid())
	{
		mIsPlaying = false;
		return 0;
	}

	if (Channels == 1) // We just want half the samples.
		toRead >>= 1;
	
	if (PaUtil_GetRingBufferReadAvailable(&mRingBuf) < toRead || !mIsPlaying)
		toRead = PaUtil_GetRingBufferReadAvailable(&mRingBuf);

	if (mIsPlaying)
	{
		if (mSource->GetRate() != 44100.0 || mPitch != 1) // Alright, we'll need to resample. Let's just do that.
		{
			// This is what our destination rate will be
			double origRate = mSource->GetRate();

			// This is what our destination rate is.
			double resRate = 44100.0 / mPitch;
			double RateRatio = resRate / origRate;

			// This is how many samples we want to read from the source buffer
			size_t scount = ceil(origRate * toRead / resRate);

			cnt = PaUtil_ReadRingBuffer(&mRingBuf, mResampleBuffer.data(), scount);
			// cnt now contains how many samples we actually read... 

			// This is how many resulting samples we can output with what we read...
			outcnt = floor(cnt * resRate / origRate);


			size_t odone;

			if (Channels == 1) // Turn mono audio to stereo audio.
				monoToStereo(mResampleBuffer.data(), cnt, BUFF_SIZE);

			soxr_set_io_ratio(mResampler, 1 / RateRatio, cnt / 2);

			// The count that soxr asks for I think, is frames, not samples. Thus, the division by channels.
			soxr_process(mResampler, 
						mResampleBuffer.data(), cnt / Channels, nullptr, 
						buffer, outcnt / Channels, &odone);

			outcnt = odone;
		}
		else
		{
			cnt = PaUtil_ReadRingBuffer(&mRingBuf, buffer, toRead);

			if (Channels == 1)
				monoToStereo(buffer, cnt, BUFF_SIZE);

			outcnt = cnt;
		}

		mStreamTime += double(cnt/Channels) / mSource->GetRate();
		mPlaybackTime = mStreamTime - MixerGetLatency();
	}else
		return 0;

	return outcnt;
}

bool AudioStream::Open(const char* Filename)
{
	mSource = SourceFromExt(RearrangeFilename(Filename));

	if (mSource && mSource->IsValid())
	{
		Channels = mSource->GetChannels();
		
		
		mResampleBuffer.resize(BUFF_SIZE);

		soxr_io_spec_t sis;
		sis.flags = 0;
		sis.itype = SOXR_FLOAT32_I;
		sis.otype = SOXR_FLOAT32_I;
		sis.scale = 1;
		soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_VHQ, SOXR_VR);
		mResampler = soxr_create(mSource->GetRate(), 44100, 2, nullptr, &sis, &q_spec, nullptr);
		
		mBufferSize = BUFF_SIZE;
		mData.resize(mBufferSize);
		PaUtil_InitializeRingBuffer(&mRingBuf, sizeof(float), mBufferSize, mData.data());

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
	mSource->Seek(float(Sample) / mSource->GetRate());
}

void AudioStream::Stop()
{
	mIsPlaying = false;
}

uint32 AudioStream::Update()
{
	float tbuf[BUFF_SIZE];
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
