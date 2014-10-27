Below are a bunch of explanations as to what keys on the ini file do what.

DisableBMP:
If enabled, disables BMS BGAs.

Widescreen:
Enable Widescreen
16:9 is 1, 16:10 is 2, anything else is 4:3

OffsetDC/Offset7K:
Mode offsets, adds this much time in seconds to all notes

FlushVideo:
Finish all OpenGL commands before swapping

VSync:
Enable Vertical syncronization

Fullscreen:
Enables and disables fullscreen at startup.

WindowWidth/WindowHeight:
Sets resolution at which fullscreen and the initial window are set.

DisableHitsounds:
Disables hitsounds for osu!mania charts. Does not affect BMS or keysounded osu!mania charts.

InterpolateTime:
When using a song with streamed music rather than samples, when enabled, tries to interpolate between audio frames rather
than staying 100% on sync with the music.

UseWasapi:
Enables wasapi on windows only. When disabled, defaults to directsound.

WasapiDontUseExclusiveMode:
When enabled and UseWasapi is in effect, tries to not use exclusive mode.

AudioCompensation:
Try to automatically sync charts.

UseAudioCompensationKeysounds:
Applies audio latency compensation to keysounded charts. Only when AudioCompensation is enabled!

UseAudioCompensationNonKeysounded:
Applies audio latency compensation to non keysounded charts. Only when AudioCompensation is enabled!

DontUseLowLatency: 
Uses high latency values provided by portaudio rather than the low latency values. Mostly useful with ALSA (linux)

UseThreadedDecoder:
When decoding audio, use a separate thread instead of the main thread.

OggListing:
Show a list of ogg files for editing with raindrop's dotcur mode.



