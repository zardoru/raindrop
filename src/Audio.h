#ifndef AUDIO_H_
#define AUDIO_H_

#include "Audiofile.h"

void InitAudio();

#define BUFF_SIZE 4096

#define SoundStream AudioStream
#define SoundSample	AudioSample

GString GetOggTitle(GString file);

void MixerAddStream(SoundStream *Sound);
void MixerRemoveStream(SoundStream* Sound);
void MixerAddSample(SoundSample *Sound);
void MixerRemoveSample(SoundSample* Sound);
void MixerUpdate();
double MixerGetLatency();
double MixerGetFactor();

#endif // AUDIO_H_