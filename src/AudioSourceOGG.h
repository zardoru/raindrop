#ifndef OGGFILESRC_H_
#define OGGFILESRC_H_

#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

class AudioSourceOGG : public AudioDataSource
{
	OggVorbis_File mOggFile;
	bool mIsValid;
	vorbis_info* info;
	vorbis_comment* comment;
	float mSeekTime;
	bool mIsDataLeft;

public:
	AudioSourceOGG();
	~AudioSourceOGG();
	bool Open(const char* Filename);
	uint32 Read(void* buffer, size_t count);
	void Seek(float Time);
	size_t GetLength(); // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate(); // Returns sampling rate of audio
	uint32 GetChannels(); // Returns channels of audio
	bool IsValid();
	bool HasDataLeft();
};

#endif