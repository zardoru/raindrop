#ifndef AUDIO_H_
#define AUDIO_H_

#include <portaudio.h>
#include "pa_ringbuffer.h"
#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

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
	uint32 BufSize;
	PaUtilRingBuffer RingBuf;
	bool runThread;
	bool loop;
	bool threadRunning; // status report. 
	bool isOpen;
	friend class PaStreamWrapper;
	friend class PaMixer;
	boost::thread *thread;

	void VorbisStream::clearBuffer();
	char tbuf[BUFF_SIZE*sizeof(int16)];
	void operator () ();

	double SeekTime;
	double playbackTime, streamTime;

public:
	VorbisStream(const char* Filename, uint32 bufferSize = BUFF_SIZE);
	VorbisStream(FILE *fp, uint32 bufferSize = BUFF_SIZE);

	~VorbisStream();

	void startStream();
	void stopStream();

	double getRate();
	int32 getChannels();
	int32 readBuffer(void * out, uint32 length);
	void seek(double Time, bool accurate = false); /* accurate also means thread safe */
	bool IsOpen();
	void setLoop(bool _loop);
	void Start();
	void Stop();
	bool IsStopped();
	double GetPlaybackTime();
	double GetStreamedTime();

	void UpdateBuffer(int32 &read);
};

class VorbisSample
{
	OggVorbis_File f;
	vorbis_info* info;
	vorbis_comment* comment;
	char* buffer;
	uint32 BufSize;
	uint32 Counter;
	boost::mutex cntmux;
public:
	VorbisSample(const char* filename);
	~VorbisSample();
	double getRate();
	int32 getChannels();
	int32 readBuffer(void * out, uint32 length);
	bool IsOpen();
	void Reset();
};

class PaStreamWrapper
{
	PaStreamParameters outputParams;
	PaStream* mStream;
	VorbisStream *Sound;
public:

	PaStreamWrapper(const char* filename);
	PaStreamWrapper(VorbisStream *Vs);

	~PaStreamWrapper();

	bool IsValid();
	void Start(bool looping = false, bool Stream = true);
	void Stop();
	void Restart();
	void Seek(double Time, bool Accurate = true, bool RestartStream = true);
	double GetPlaybackTime();
	bool IsStopped();

	VorbisStream *GetStream();
};

double GetDeviceLatency();
std::string GetOggTitle(std::string file);

#define SoundStream PaStreamWrapper
#define SoundSample	VorbisSample

void MixerAddStream(VorbisStream *Sound);
void MixerRemoveStream(VorbisStream* Sound);
void MixerAddSample(VorbisSample *Sound);
void MixerRemoveSample(VorbisSample* Sound);
#endif // AUDIO_H_