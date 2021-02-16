#include "av-io-common-pch.h"
#include "Audiofile.h"
#include "AudioSourceSFM.h"

#ifdef WIN32
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif


#include <sndfile.h>
#include "TextAndFileUtil.h"


AudioSourceSFM::AudioSourceSFM()
{
    mWavFile = nullptr;
    info = nullptr;
    mIsDataLeft = false;
    mFlen = 0;
}

AudioSourceSFM::~AudioSourceSFM()
{
    if (mWavFile)
        sf_close((SNDFILE*)mWavFile);

    delete info;
}

bool AudioSourceSFM::Open(std::filesystem::path Filename)
{
    if (info) // we're already in use
        return false;

    info = new SF_INFO;
    info->format = 0;

#ifndef WIN32
    mWavFile = sf_open(Conversion::ToU8(Filename.wstring()).c_str(), SFM_READ, info);
#else
    mWavFile = sf_wchar_open(Filename.wstring().c_str(), SFM_READ, info);
#endif

    mRate = info->samplerate;
    mChannels = info->channels;
    mFlen = info->frames * info->channels;

    int err = 0;
    if (!mWavFile || (err = sf_error((SNDFILE*)mWavFile)))
    {
        // Log::LogPrintf("Error %d: Filename %s (wavfile %p)\n", err, Utility::ToU8(Filename.wstring()).c_str(), mWavFile);
        return false;
    }

    // sf_command(mWavFile, SFC_SET_SCALE_FLOAT_INT_READ, NULL, SF_TRUE);
    mIsDataLeft = true;
    return true;
}

uint32_t AudioSourceSFM::Read(short* buffer, size_t count)
{
    auto read = 0U;
    if (mWavFile)
    {
        read = sf_read_short((SNDFILE*)mWavFile, buffer, count);
        int remaining = count - read;

        while (mSourceLoop && (remaining > 0) && read)
        {
            Seek(0);
            read += sf_read_short((SNDFILE*)mWavFile, static_cast<short*>(buffer) + (read), remaining);
            remaining -= read;
        }

        if (read <= 0)
            mIsDataLeft = false;
    }

    return read;
}

void AudioSourceSFM::Seek(float Time)
{
    if (mWavFile)
    {
        mIsDataLeft = true;

        if (Time * mRate <= mFlen && info->seekable)
            sf_seek((SNDFILE*)mWavFile, Time * mRate, SEEK_SET);
    }
}

size_t AudioSourceSFM::GetLength()
{
    return info->frames;
}

uint32_t AudioSourceSFM::GetRate()
{
    return mRate;
}

uint32_t AudioSourceSFM::GetChannels()
{
    return mChannels;
}

bool AudioSourceSFM::IsValid()
{
    return mWavFile != nullptr;
}

bool AudioSourceSFM::HasDataLeft()
{
    return mIsDataLeft;
}
