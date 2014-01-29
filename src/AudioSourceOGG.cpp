#include "Global.h"
#include "Audiofile.h"
#include "AudioSourceOGG.h"

AudioSourceOGG::AudioSourceOGG()
{ 
	mIsValid = false;
	mSourceLoop = false;
}
AudioSourceOGG::~AudioSourceOGG()
{
	if (mIsValid)
		ov_clear(&mOggFile);
}

bool AudioSourceOGG::Open(const char* Filename)
{
	int32 retv = ov_fopen(Filename, &mOggFile);

	if (retv == 0)
	{
		info = ov_info(&mOggFile, -1);
		comment = ov_comment(&mOggFile, -1);

		mIsValid = true;
	}else
		mIsValid = false;

	return mIsValid;
}

uint32 AudioSourceOGG::Read(void* buffer, size_t count)
{
	int32 size;
	int32 read = 0, sect;

	if (!mIsValid)
		return 0;

	size = count*sizeof(uint16);

	if (mSeekTime >= 0)
	{
		ov_time_seek(&mOggFile, mSeekTime);
		mSeekTime = -1;
	}

	/* read from ogg vorbis file */
	int32 res = 1;
	while (read < size)
	{
		res = ov_read(&mOggFile, (char*)buffer+read, size - read, 0, 2, 1, &sect);

		if (res > 0)
			read += res;

		else if (res == 0)
		{
			if (mSourceLoop)
			{
				ov_time_seek(&mOggFile, 0);
				continue;
			}
			else
				return 0;
		}
		else 
			return 0;
	}

	return read || res != 0;
}

void AudioSourceOGG::Seek(float Time)
{ 
	mSeekTime = Time;
}

size_t AudioSourceOGG::GetLength()
{ 
	return ov_pcm_total(&mOggFile, -1);
}

uint32 AudioSourceOGG::GetRate()
{ 
	return info->rate;
}

uint32 AudioSourceOGG::GetChannels()
{ 
	return info->channels;
}

bool AudioSourceOGG::IsValid()
{ 
	return mIsValid;
}


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

String GetOggTitle(String file)
{
	OggVorbis_File f;
	String result = "";
	if (ov_fopen(file.c_str(), &f) == 0)
	{
		vorbis_comment *comment = ov_comment(&f, -1);

		for (int i = 0; i < comment->comments; i++)
		{
			std::vector<String> splitvec;
			std::string user_comment = comment->user_comments[i];
			boost::split(splitvec, user_comment, boost::is_any_of("="));
			if (splitvec[0] == "TITLE")
			{
				result = splitvec[1].c_str();
				break;
			}
		}

		ov_clear(&f);
	}

	return result;
}