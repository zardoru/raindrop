#pragma once


#include <string>
#include <sndio/IMixer.h>

void init_audio();
void update_mixer();
double mixer_get_time();
double mixer_get_latency();

IMixer* get_mixer();
