#ifndef AUDIO_H_
#define AUDIO_H_

#include <portaudio.h>
#include "pa_ringbuffer.h"
#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

void InitAudio();
void StartStream(const char* sound);

#define BUFF_SIZE 8192


/* having to show our privates is pretty annoying. */
class VorbisStream
{
	OggVorbis_File f;
	vorbis_info* info;
	vorbis_comment* comment;
	char* buffer;
	int BufSize;
	PaUtilRingBuffer RingBuf;
	bool runThread;
	bool loop;

	friend class PaStreamWrapper;

	void VorbisStream::clearBuffer();
	char tbuf[BUFF_SIZE*sizeof(short int)];
	void UpdateBuffer(int &read);
	void operator () ();

public:
	VorbisStream(FILE *fp, int bufferSize = BUFF_SIZE);

	~VorbisStream();

	double getRate();
	int getChannels();
	int readBuffer(void * out, int length, const PaStreamCallbackTimeInfo *timeInfo);
};

class VorbisStream;

class PaStreamWrapper
{
	PaStreamParameters outputParams;
	PaStream* mStream;
	VorbisStream *Sound;
public:

	PaStreamWrapper(char* filename);
	PaStreamWrapper(VorbisStream *Vs);

	~PaStreamWrapper();

	void Start(bool looping = false);
};

#define SoundStream PaStreamWrapper


#endif // AUDIO_H_