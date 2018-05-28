#include "pch.h"

#include "Logging.h"
#include "Audio.h"
#include "AudioSourceMP3.h"

static bool mpg123_initialized = false;

AudioSourceMP3::AudioSourceMP3()
{
    int err;

    if (!mpg123_initialized)
    {
        mpg123_init();
        mpg123_initialized = true;
    }

    mHandle = mpg123_new(nullptr, &err);
    mpg123_format_none(mHandle);
    mIsValid = false;
    mIsDataLeft = false;
}

AudioSourceMP3::~AudioSourceMP3()
{
    mpg123_close(mHandle);
    mpg123_delete(mHandle);
}

bool AudioSourceMP3::Open(std::filesystem::path Filename)
{
    mpg123_param(mHandle, MPG123_FORCE_RATE, MixerGetRate(), 1);

#if !(defined WIN32) || (defined MINGW)
    if (mpg123_open(mHandle, Utility::ToU8(Filename.wstring()).c_str()) == MPG123_OK)
#else
    if (mpg123_topen(mHandle, Filename.c_str()) == MPG123_OK)
#endif
    {
        long rate;

        mpg123_format_all(mHandle);
        mpg123_format(mHandle, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);

        mpg123_getformat(mHandle, &rate, &mChannels, &mEncoding);

        mRate = rate;

        size_t pos = mpg123_tell(mHandle);
        size_t start = mpg123_seek(mHandle, 0, SEEK_SET);
        size_t end = mpg123_seek(mHandle, 0, SEEK_END);
        mpg123_seek(mHandle, pos, SEEK_SET);

        mLen = end - start;

        mIsValid = true;
        mIsDataLeft = true;
        return true;
	}
	else Log::LogPrintf("Failure loading MP3 file '%s'\n", Filename.string().c_str());

    return false;
}

uint32_t AudioSourceMP3::Read(short* buffer, size_t count)
{
    size_t actuallyread;
    auto toRead = count * sizeof(short); // # of bytes to actually read

    if (toRead == 0 || !IsValid())
        return 0;

    // read # bytes into varr.

    auto res = mpg123_read(mHandle, reinterpret_cast<unsigned char*>(buffer), toRead, &actuallyread);
    size_t last_read = actuallyread;

    while (mSourceLoop && actuallyread < toRead)
    {
		if (res == MPG123_DONE) {
			Seek(0);
		}

        res = mpg123_read(mHandle, reinterpret_cast<unsigned char*>(buffer) + actuallyread, toRead - actuallyread, &last_read);
        actuallyread += last_read;
    }

    // according to mpg123_read documentation, ret is the amount of bytes read. We want to return samples read.
    return actuallyread / sizeof(short);
}

void AudioSourceMP3::Seek(float Time)
{
    int place = round(mRate * Time);

	if (place > mLen) {
		Log::Printf("Attempt to seek after the stream's end.\n");
		return;
	}

    int res = mpg123_seek(mHandle, place, SEEK_SET);
    if (res < 0 || res < place) {
        Log::Printf("Error seeking stream at %f (tried %d, got %d)\n", Time, place, res);
		return;
    }

    mIsDataLeft = true;
}

size_t AudioSourceMP3::GetLength()
{
    return mLen;
}

uint32_t AudioSourceMP3::GetRate()
{
    return mRate;
}

uint32_t AudioSourceMP3::GetChannels()
{
    return mChannels;
}

bool AudioSourceMP3::IsValid()
{
    if (!mIsValid) return false;

    auto err = mHandle ? mpg123_errcode(mHandle) : MPG123_OK;
    auto isValidMp3Stream = (err == MPG123_OK ||
        err == MPG123_DONE ||
        err == MPG123_ERR_READER); // This third one is because valid streams throw off this error.
    if (!isValidMp3Stream)
    {
        Log::Printf("Mp3 Decoder error: %s\n", mpg123_strerror(mHandle));
        mIsValid = false;
    }

    return mHandle && isValidMp3Stream;
}

bool AudioSourceMP3::HasDataLeft()
{
    return mIsDataLeft;
}

AudioSourceMP3::Metadata AudioSourceMP3::GetMetadata()
{
	if (mpg123_scan(mHandle) == MPG123_OK)
	{
		auto meta = mpg123_meta_check(mHandle);
		mpg123_id3v1 *v1;
		mpg123_id3v2 *v2;

		if ((meta & MPG123_ID3) && 
			mpg123_id3(mHandle, &v1, &v2) == MPG123_OK) {
			if (v1) {
				return Metadata{
					std::string(v1->artist, 30),
					std::string(v1->title, 30)
				};
			}

			if (v2) {
				return Metadata{
					std::string(v2->artist ? v2->artist->p : "", 30),
					std::string(v2->title ? v2->title->p : "", 30)
				};
			}
		}
	}


	return Metadata();
}

