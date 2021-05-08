#include "av-io-common-pch.h"
#include "Audiofile.h"
#include "AudioSourceMP3.h"

#include <fstream>

#include "TextAndFileUtil.h"

#ifdef WIN32

#ifndef MINGW
#include <direct.h>
#include <mpg123.h>
#else
#include <mpg123mingw.h>
#endif

#endif

#ifdef LINUX
//This one might be needed : #include <fcntl.h>
#include <mpg123.h>
#include <unistd.h>
#include <pa_linux_alsa.h>
#endif

static bool mpg123_initialized = false;

ssize_t read_mp3(void* opaque, void* buf, size_t buf_size) {
    auto& me = *reinterpret_cast<std::ifstream*>(opaque);
    me.read(reinterpret_cast<char*>(buf), buf_size);
    return me.gcount();
}

off_t seek_mp3(void* opaque, off_t off, int whence) {
    auto& me = *reinterpret_cast<std::ifstream*>(opaque);
    switch (whence) {
        case SEEK_CUR:
            me.seekg(off, std::ios::cur);
            break;
        case SEEK_END:
            me.seekg(off, std::ios::end);
            break;
        case SEEK_SET:
            me.seekg(off, std::ios::beg);
            break;
        default:
            break;
    }

    auto tg = me.tellg();
    return tg;
}

void cleanup_mp3(void* opaque) {
    auto* me = reinterpret_cast<std::ifstream*>(opaque);
    delete me;
}


AudioSourceMP3::AudioSourceMP3()
{
    int err;

    if (!mpg123_initialized)
    {
        mpg123_init();
        mpg123_initialized = true;
    }

    mHandle = mpg123_new(nullptr, &err);
    mpg123_replace_reader_handle(
        (mpg123_handle*)mHandle,
        read_mp3,
        seek_mp3,
        cleanup_mp3
    );

    mpg123_format_none((mpg123_handle*)mHandle);
    mIsValid = false;
    mIsDataLeft = false;
    ioHandle = nullptr;
}

AudioSourceMP3::~AudioSourceMP3()
{
    mpg123_close((mpg123_handle*)mHandle);
    mpg123_delete((mpg123_handle*)mHandle);
}

bool AudioSourceMP3::Open(std::filesystem::path Filename)
{
    // if (mOwnerMixer)
    // mpg123_param((mpg123_handle*)mHandle, MPG123_FORCE_RATE, MixerGetRate(), 1);
    auto ios = new std::ifstream (Filename, std::ios::binary);

    if (!ios->is_open()) {
        return false;
    }

    ioHandle = ios;

    if (mpg123_open_handle((mpg123_handle*)mHandle, ioHandle) == MPG123_OK)
    {
        long rate;

        auto res = mpg123_format_all((mpg123_handle*)mHandle);
        // mpg123_format((mpg123_handle*)mHandle, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);

        auto res2 = mpg123_getformat((mpg123_handle*)mHandle, &rate, &mChannels, &mEncoding);

        mRate = rate;

        mLen = mpg123_length((mpg123_handle*)mHandle);

        mIsValid = true;
        mIsDataLeft = mLen > 0;
        return true;
	}
	//else
	//	Log::LogPrintf("Failure loading MP3 file '%s'\n", Filename.string().c_str());

    return false;
}

uint32_t AudioSourceMP3::Read(short* buffer, size_t count)
{
    size_t actuallyread;
    auto toRead = count * sizeof(short); // # of bytes to actually read

    if (toRead == 0 || !IsValid())
        return 0;

    // read # bytes into varr.

    auto res = mpg123_read((mpg123_handle*)mHandle, reinterpret_cast<unsigned char*>(buffer), toRead, &actuallyread);
    size_t last_read = actuallyread;

	if (res == MPG123_DONE && !mSourceLoop) {
		mIsDataLeft = false;
	}

    while (mSourceLoop && actuallyread < toRead)
    {
		if (res == MPG123_DONE) {
			Seek(0);
		}

        res = mpg123_read(
            (mpg123_handle*)mHandle, 
            reinterpret_cast<unsigned char*>(buffer) + actuallyread, 
            toRead - actuallyread, 
            &last_read
        )
        ;
        actuallyread += last_read;
    }

    // according to mpg123_read documentation, ret is the amount of bytes read. We want to return samples read.
    return actuallyread / sizeof(short);
}

void AudioSourceMP3::Seek(float Time)
{
    int place = round(mRate * Time);

	if (place > mLen) {
		// Log::Printf("Attempt to seek after the stream's end.\n");
		return;
	}

    int res = mpg123_seek((mpg123_handle*)mHandle, place, SEEK_SET);
    if (res < 0 || res < place) {
        // Log::Printf("Error seeking stream at %f (tried %d, got %d)\n", Time, place, res);
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

    auto err = mHandle ? mpg123_errcode((mpg123_handle*)mHandle) : MPG123_OK;
    auto isValidMp3Stream = (err == MPG123_OK ||
        err == MPG123_DONE ||
        err == MPG123_ERR_READER); // This third one is because valid streams throw off this error.
    if (!isValidMp3Stream)
    {
        // Log::Printf("Mp3 Decoder error: %s\n", mpg123_strerror(mHandle));
        mIsValid = false;
    }

    return mHandle && isValidMp3Stream;
}

bool AudioSourceMP3::HasDataLeft()
{
    return mIsDataLeft;
}

std::string toSafeString1(char* s)
{
	std::string ret;
	for (int i = 0; i < 30; i++)
	{
		if (isprint(s[i]))
			ret += s[i];
		else
			return ret;
	}

	return ret;
}

std::string toSafeString2(mpg123_string *str)
{
	std::string s;
	if (str) {
		if (str->p != nullptr) {
			s = std::string(str->p, str->fill - 1);
		}
	}

	return s;
}

AudioSourceMP3::Metadata AudioSourceMP3::GetMetadata()
{
	if (mpg123_scan((mpg123_handle*)mHandle) == MPG123_OK)
	{
		auto meta = mpg123_meta_check((mpg123_handle*)mHandle);
		mpg123_id3v1 *v1;
		mpg123_id3v2 *v2;

		if ((meta & MPG123_ID3) && 
			mpg123_id3((mpg123_handle*)mHandle, &v1, &v2) == MPG123_OK) {
			if (v1) {
				return Metadata{
					toSafeString1(v1->artist),
					toSafeString1(v1->title)
				};
			}

			if (v2) {
				return Metadata{
					toSafeString2(v2->artist),
					toSafeString2(v2->title)
				};
			}
		}
	}


	return Metadata();
}

