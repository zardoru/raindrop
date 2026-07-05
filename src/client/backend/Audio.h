#pragma once


#include <string>
#include <sndio/IMixer.h>

void init_audio();
void update_mixer();
double MixerGetTime();
double MixerGetLatency();

IMixer* GetMixer();
