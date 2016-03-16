#pragma once

#include "Audiofile.h"

void InitAudio();

#define BUFF_SIZE 8192

#define SoundStream AudioStream
#define SoundSample	AudioSample

std::string GetOggTitle(std::string file);

void MixerAddStream(SoundStream *Sound);
void MixerRemoveStream(SoundStream* Sound);
void MixerAddSample(SoundSample *Sound);
void MixerRemoveSample(SoundSample* Sound);
void MixerUpdate();
double MixerGetLatency();
double MixerGetRate();
double MixerGetFactor();
double MixerGetTime();