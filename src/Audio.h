#pragma once

#include "Audiofile.h"

void InitAudio();

#define BUFF_SIZE 8192


std::string GetOggTitle(std::string file);

void MixerAddStream(AudioStream *Sound);
void MixerRemoveStream(AudioStream* Sound);
void MixerAddSample(AudioSample *Sound);
void MixerRemoveSample(AudioSample* Sound);
void MixerUpdate();
double MixerGetLatency();
double MixerGetRate();
double MixerGetFactor();
double MixerGetTime();