#include "av-io-common-pch.h"

#include "Audiofile.h"
#include "IMixer.h"

#include "AudioSourceOGG.h"
#include <text_and_file_util.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

class AudioSourceOGG::AudioSourceOGGInternal {
public:
    OggVorbis_File mOggFile;

    vorbis_info* info;
    vorbis_comment* comment;

    AudioSourceOGGInternal() {
        info = nullptr;
        comment = nullptr;
    }
};


/*
    you may ask, "why do you use libogg directly instead of libsndfile?"
    and I can only respond - ogg on libsndfile is broken as hell.
*/

size_t readOGG(void* ptr, const size_t size, const size_t nmemb, void* p)
{
    FILE* fp = static_cast<FILE*>(p);
    return fread(ptr, size, nmemb, fp);
}

int seekOGG(void* p, const ogg_int64_t offs, const int whence)
{
    FILE* fp = static_cast<FILE*>(p);
    return fseek(fp, offs, whence);
}

long tellOGG(void* p)
{
    FILE* fp = static_cast<FILE*>(p);
    return ftell(fp);
}

int closeOgg(void* p)
{
    return fclose(static_cast<FILE*>(p));
}

ov_callbacks fileInterfaceOgg = {
    readOGG,
    seekOGG,
    closeOgg,
    tellOGG
};

AudioSourceOGG::AudioSourceOGG()
{
    mIsValid = false;
    mSourceLoop = false;
    mIsDataLeft = false;
    mSeekTime = -1;

    internal = std::make_unique<AudioSourceOGGInternal>();
}

AudioSourceOGG::~AudioSourceOGG()
{
    if (mIsValid)
        ov_clear(&internal->mOggFile);
}

bool AudioSourceOGG::open(const std::filesystem::path Filename)
{
#if !(defined WIN32) || (defined MINGW)
    int32_t retv = ov_fopen(otoworm::locale::wstring_to_utf8(Filename.wstring()).c_str(), &internal->mOggFile);
#else
    FILE* fp = _wfopen(Filename.wstring().c_str(), L"rb");
    int retv = -1;

    if (fp)
        retv = ov_open_callbacks(static_cast<void*>(fp), &internal->mOggFile, nullptr, 0, fileInterfaceOgg);
#endif

#if !(defined WIN32) || (defined MINGW)
    if (retv == 0)
#else
    if (retv == 0 && fp)
#endif
    {
        internal->info = ov_info(&internal->mOggFile, -1);
        internal->comment = ov_comment(&internal->mOggFile, -1);

        mIsValid = internal->info && internal->info->rate > 0 && internal->info->channels >= 1 && internal->info->channels <= 2;
        mIsDataLeft = mIsValid;

        if (!mIsValid)
            ov_clear(&internal->mOggFile);
    }
    else
    {
        mIsValid = false;
        // Log::LogPrintf("Failure loading ogg file: %s (%d)\n", otoworm::locale::wstring_to_utf8(Filename.wstring()).c_str(), retv);
    }

    return mIsValid;
}

uint32_t AudioSourceOGG::read(short* buffer, const size_t count)
{
    size_t size;
    size_t read = 0;
    int sect;

    if (!mIsValid)
        return 0;

    size = count*sizeof(short);

    if (mSeekTime >= 0)
    {
        mIsDataLeft = true;
        ov_time_seek(&internal->mOggFile, mSeekTime);
        mSeekTime = -1;
    }

    /* read from ogg vorbis file */
    while (read < size)
    {
        const long res = ov_read(&internal->mOggFile, reinterpret_cast<char*>(buffer) + read, size - read, 0, 2, 1, &sect);

        if (res > 0)
            read += res;

        else if (res == 0)
        {
            if (mSourceLoop)
            {
                ov_time_seek(&internal->mOggFile, 0);
            }
            else
            {
                mIsDataLeft = false;
                return read / sizeof(short);
            }
        }
        else
        {
            // Log::Printf("AudioSourceOGG: Error while reading OGG source (%d)\n", res);
            mIsDataLeft = false;
            return read / sizeof(short);
        }
    }

    return read / sizeof(short);
}

void AudioSourceOGG::seek(const float Time)
{
    mSeekTime = Time;
}

size_t AudioSourceOGG::get_length()
{
    return ov_pcm_total(&internal->mOggFile, -1);
}

uint32_t AudioSourceOGG::get_rate()
{
    return internal->info->rate;
}

uint32_t AudioSourceOGG::get_channels()
{
    return internal->info->channels;
}

bool AudioSourceOGG::is_valid()
{
    return mIsValid;
}

bool AudioSourceOGG::has_data_left()
{
    return mIsDataLeft;
}

std::string GetOggTitle(std::string file)
{
    OggVorbis_File f;
    std::string result = "";
    if (ov_fopen(file.c_str(), &f) == 0)
    {
        vorbis_comment *comment = ov_comment(&f, -1);

        for (int i = 0; i < comment->comments; i++)
        {
            std::string user_comment = comment->user_comments[i];
            auto splitvec = otoworm::util::token_split(user_comment, "=");
            if (splitvec[0] == "TITLE")
            {
                result = splitvec[1];
                break;
            }
        }

        ov_clear(&f);
    }

    return result;
}
