#pragma once


#include <string>
#include <sndio/IMixer.h>

void InitAudio();
void MixerUpdate();
double MixerGetTime();
double MixerGetLatency();

IMixer* GetMixer();
