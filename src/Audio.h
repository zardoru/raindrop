#ifndef AUDIO_H_
#define AUDIO_H_

#include <portaudio.h>
#include "pa_ringbuffer.h"
#include "Audiofile.h"

void InitAudio();
void StartStream(const char* sound);

#define BUFF_SIZE 4096


#define SoundStream AudioStream
#define SoundSample	AudioSample

class PaStreamWrapper
{
	PaStreamParameters outputParams;
	PaStream* mStream;
	AudioStream *Sound;
public:

	PaStreamWrapper(const char* filename);
	PaStreamWrapper(AudioStream *Vs);

	~PaStreamWrapper();

	bool IsValid();
	void Start(bool looping = false);
	void Stop();
	void Restart();
	void Seek(double Time);
	double GetPlaybackTime();
	double GetStreamTime();
	bool IsStopped();

	AudioStream *GetStream();
};

double GetDeviceLatency();
String GetOggTitle(String file);

void MixerAddStream(SoundStream *Sound);
void MixerRemoveStream(SoundStream* Sound);
void MixerAddSample(SoundSample *Sound);
void MixerRemoveSample(SoundSample* Sound);
void MixerUpdate();

#endif // AUDIO_H_