#pragma once

#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

class AudioSourceOGG : public AudioDataSource
{
	OggVorbis_File mOggFile;

	vorbis_info* info;
	vorbis_comment* comment;
	float mSeekTime;

	bool mIsValid;
	bool mIsDataLeft;

public:
	AudioSourceOGG();
	~AudioSourceOGG();
	bool Open(const char* Filename) override;
	uint32 Read(short* buffer, size_t count) override;
	void Seek(float Time) override;
	size_t GetLength() override; // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate() override; // Returns sampling rate of audio
	uint32 GetChannels() override; // Returns channels of audio
	bool IsValid() override;
	bool HasDataLeft() override;
};