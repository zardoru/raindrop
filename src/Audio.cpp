#include "Global.h"
#include "Audio.h"
#include "Configuration.h"
#include <cstdio>
#include <cstring>

#ifdef WIN32
#include <pa_win_wasapi.h>
#endif

#ifdef LINUX
#include <pa_linux_alsa.h>
#endif

#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
float VolumeSFX = 0.1f;
float VolumeMusic = 0.8f; 
bool UseThreadedDecoder = false;

#ifdef WIN32
bool UseWasapi = false;
PaDeviceIndex DefaultWasapiDevice;
#endif

/* Vorbis file stream

static int32 StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	bool cont = !((SoundStream*)(userData))->Read(output, frameCount * 2);
	return cont;
}

 */
/*************************/
/********* Mixer *********/
/*************************/

PaError OpenStream(PaStream **mStream, PaDeviceIndex Device, double Rate, void* Sound, double &dLatency, PaStreamCallback Callback)
{
		PaStreamParameters outputParams;

		outputParams.device = Device;
		outputParams.channelCount = 2;
		outputParams.sampleFormat = paInt16;

		if (!Configuration::GetConfigf("DontUseLowLatency"))
			outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
		else
			outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
			

#ifndef WIN32

		outputParams.hostApiSpecificStreamInfo = NULL;

#else
		PaWasapiStreamInfo StreamInfo;
		if (UseWasapi)
		{
			outputParams.hostApiSpecificStreamInfo = &StreamInfo;
			StreamInfo.hostApiType = paWASAPI;
			StreamInfo.size = sizeof(PaWasapiStreamInfo);
			StreamInfo.version = 1;
			if (!Configuration::GetConfigf("WasapiDontUseExclusiveMode"))
			{
				StreamInfo.threadPriority = eThreadPriorityProAudio;
				StreamInfo.flags = paWinWasapiExclusive;
			}
			else
			{
				StreamInfo.threadPriority = eThreadPriorityGames;
				StreamInfo.flags = 0; 
			}
	
			StreamInfo.hostProcessorOutput = NULL;
			StreamInfo.hostProcessorInput = NULL;
		}else
			outputParams.hostApiSpecificStreamInfo = NULL;
#endif

	dLatency = outputParams.suggestedLatency;

	// fire up portaudio
	PaError Err = Pa_OpenStream(mStream, NULL, &outputParams, Rate, 0, paClipOff + paDitherOff, Callback, (void*)Sound);

	if (Err)
	{
		printf("%s\n", Pa_GetErrorText(Err));
		printf("Device Selected %d\n", Device);
	}
#ifdef LINUX
	else
	{
		printf("Audio: Enabling real time scheduling\n");
		PaAlsa_EnableRealtimeScheduling( mStream, true );
	}	
#endif

	return Err;
}

#ifdef WIN32
PaDeviceIndex GetWasapiDevice()
{
	return DefaultWasapiDevice; // implement properly?
}
#else
PaDeviceIndex GetWasapiDevice()
{
	return Pa_GetDefaultOutputDevice();
}
#endif

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

class PaMixer
{
	PaStream* Stream;
	char *RingbufData;
	PaUtilRingBuffer RingBuf;

	double Latency;

	std::vector<SoundStream*> Streams;
	std::vector<SoundSample*> Samples;
	double ConstFactor;

	void MixBuffer(char* Src, char* Dest, int Length, const int Start, const float Multiplier)
	{
		Src += Start;

		while (Length)
		{
			float A = ((float)(*Src) * Multiplier + (float)*Dest);

			if (A > 127)
			{
				A = 127;
			}
			if (A < -128)
			{
				A = -128;
			}


			*Dest = A;

			Dest++;
			Src++;
			Length--;
		}
	}

	int SizeAvailable;
	bool Threaded;

	boost::mutex mut, mut2, rbufmux;
	boost::condition ringbuffer_has_space;
public:
	PaMixer(bool StartThread)
	{
		RingbufData = new char[BUFF_SIZE*sizeof(int16)];
		PaUtil_InitializeRingBuffer(&RingBuf, sizeof(int16), BUFF_SIZE, RingbufData);

		Threaded = StartThread;

		if (StartThread)
		{
			boost::thread (&PaMixer::Run, this);
		}
#ifdef WIN32
		if (UseWasapi)
		{
			OpenStream( &Stream, GetWasapiDevice(), 44100, (void*) this, Latency, Mix );
		}
		else
		{
			OpenStream( &Stream, Pa_GetDefaultOutputDevice(), 44100, (void*) this, Latency, Mix );
		}
#else
			OpenStream( &Stream, Pa_GetDefaultOutputDevice(), 44100, (void*) this, Latency, Mix );

#endif

		Pa_StartStream( Stream );
		ConstFactor = 1.0;
	}

	void Run()
	{
		do
		{

				if (Threaded)
					mut.lock();

				for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
				{
					if ((*i)->IsPlaying())
					{
						(*i)->Update();
					}
				}

				if (Threaded)
					mut.unlock();

		} while (Threaded);
	}

	void AppendMusic(SoundStream* Stream)
	{
		if (Threaded)
					mut.lock();
		Streams.push_back(Stream);
		if (Threaded)
					mut.unlock();
	}

	void RemoveMusic(SoundStream *Stream)
	{
		if (Threaded)
					mut.lock();
		for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			if ((*i) == Stream)
			{
				i = Streams.erase(i);
				if (i != Streams.end())
					continue;
				else
					break;
			}
		}
		if (Threaded)
				mut.unlock();
	}

	void AddSound(SoundSample* Sample)
	{
		mut2.lock();
		Samples.push_back(Sample);
		mut2.unlock();
	}

	void RemoveSound(SoundSample* Sample)
	{
		mut2.lock();
		for(std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i) == Sample)
			{
				i = Samples.erase(i);
				
				if (i == Samples.end())
					break;
			}
		}
		mut2.unlock();
	}

	private:
		char ts[BUFF_SIZE*2];
	public:

	void CopyOut(char* out, int samples)
	{
		memset(out, 0, samples * 2);

		// mut.lock();
		for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			memset(ts, 0, sizeof(ts));
			(*i)->Read(ts, samples);
			MixBuffer((char*)ts, (char*)out, samples * 2, 0, VolumeMusic);
		}
		// mut.unlock();

		// ringbuffer_has_space.notify_one();

		int Voices = 0;
		mut2.lock();
		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				Voices++;
			}
		}

		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				memset(ts, 0, sizeof(ts));
				(*i)->Read(ts, samples);
				MixBuffer((char*)ts, (char*)out, samples * 2, 0, 1.0 / sqrt((double)Voices));
			}
		}

		mut2.unlock();
	}

	double GetLatency() const
	{
		return Latency;
	}

	double GetFactor() const
	{
		return ConstFactor;
	}
};

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	PaMixer *Mix = (PaMixer*)userData;
	Mix->CopyOut((char*)output, frameCount * 2);
	return 0;
}

/****************************/
/* PortAudio Stream Wrapper */
/****************************/

PaMixer *Mixer;

PaStreamWrapper::PaStreamWrapper(SoundStream *Vs)
{
	Sound = Vs;
	mStream = NULL;
}

PaStreamWrapper::PaStreamWrapper(const char* filename)
{
	Sound = NULL;

	mStream = NULL;

	SoundStream *Vs = new SoundStream();

	if (Vs->Open(filename))
	{
		Sound = Vs;
	}else
		delete Vs;
}

PaStreamWrapper::~PaStreamWrapper()
{
	if (mStream)
		Pa_CloseStream(mStream);
}

SoundStream *PaStreamWrapper::GetStream()
{
	return Sound;
}

void PaStreamWrapper::Stop()
{
	if (mStream)
		Pa_StopStream(mStream);
}

double PaStreamWrapper::GetStreamLatency()
{
	return deviceLatency;
}

void PaStreamWrapper::Start(bool looping)
{
	if (IsValid())
	{
		Sound->SetLoop(looping);

		// start filling the ring buffer
		Sound->Play();
	}
}

void PaStreamWrapper::Seek(double Time)
{
	// this stops the sound streaming thread and flushes the ring buffer
	Sound->SeekTime(Time);
}

bool PaStreamWrapper::IsValid()
{
	return mStream && Sound;
}

double PaStreamWrapper::GetPlaybackTime()
{
	if (IsValid())
		return Sound->GetPlayedTime();
	else
		return 0;
}


double PaStreamWrapper::GetStreamTime()
{
	if (IsValid())
		return Sound->GetStreamedTime();
	else
		return 0;
}


bool PaStreamWrapper::IsStopped()
{
	return !Sound->IsPlaying();
}

/*************************/
/********** API **********/
/*************************/

void GetAudioInfo()
{
	PaHostApiIndex ApiCount = Pa_GetHostApiCount();

	printf("AUDIO: The default API is %d\n", Pa_GetDefaultHostApi());

	for (PaHostApiIndex i = 0; i < ApiCount; i++)
	{
		const PaHostApiInfo* Index = Pa_GetHostApiInfo(i);
		printf("(%d) %s: %d (%d)\n", i, Index->name, Index->defaultOutputDevice, Index->type);

#ifdef WIN32
		if (Index->type == paWASAPI)
			DefaultWasapiDevice = Index->defaultOutputDevice;
#endif
	}

	printf("\nAUDIO: The audio devices are\n");
	
	PaDeviceIndex DevCount = Pa_GetDeviceCount();
	for (PaDeviceIndex i = 0; i < DevCount; i++)
	{
		const PaDeviceInfo *Info = Pa_GetDeviceInfo(i);
		printf("(%d): %s\n", i, Info->name);
		printf("\thighLat: %f, lowLat: %f\n", Info->defaultHighOutputLatency, Info->defaultLowOutputLatency);
		printf("\tsampleRate: %f, hostApi: %d\n", Info->defaultSampleRate, Info->hostApi);
		printf("\tmaxchannels: %d\n", Info->maxOutputChannels);
	}
}

void InitAudio()
{
#ifndef NO_AUDIO
	PaError Err = Pa_Initialize();

#ifdef WIN32
	UseWasapi = (Configuration::GetConfigf("UseWasapi") != 0);
#endif

	UseThreadedDecoder = (Configuration::GetConfigf("UseThreadedDecoder") != 0);

	GetAudioInfo();

	Mixer = new PaMixer(UseThreadedDecoder);
	assert (Err == 0 && Mixer);
#endif
}

double GetDeviceLatency()
{
#ifndef NO_AUDIO
	return Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
#else
	return 0;
#endif
}

void MixerAddStream(SoundStream *Sound)
{
#ifndef NO_AUDIO
	Mixer->AppendMusic(Sound);
#endif
}

void MixerRemoveStream(SoundStream* Sound)
{
#ifndef NO_AUDIO
	Mixer->RemoveMusic(Sound);
#endif
}

void MixerAddSample(SoundSample* Sample)
{
#ifndef NO_AUDIO
	Mixer->AddSound(Sample);
#endif
}

void MixerRemoveSample(SoundSample* Sample)
{
#ifndef NO_AUDIO
	Mixer->RemoveSound(Sample);
#endif
}

void MixerUpdate()
{
#ifndef NO_AUDIO
	if (!UseThreadedDecoder)
		Mixer->Run();
#endif
}

double MixerGetLatency()
{
#ifndef NO_AUDIO
	return Mixer->GetLatency();
#else
	return 0;
#endif
}

double MixerGetFactor()
{
#ifndef NO_AUDIO
	return Mixer->GetFactor();
#else
	return 0;
#endif
}
