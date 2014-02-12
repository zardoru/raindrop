#include "Global.h"
#include "Audio.h"
#include "Configuration.h"
#include <cstdio>
#include <cstring>

#ifdef WIN32
#include <pa_win_wasapi.h>
#endif

#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
float VolumeSFX = 0.4f;
float VolumeMusic = 0.6f; 
bool UseWasapi = false;
bool UseThreadedDecoder = false;
PaDeviceIndex DefaultWasapiDevice;

/* Vorbis file stream */

static int32 StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	bool cont = !((SoundStream*)(userData))->Read(output, frameCount * 2);
	return cont;
}
/*************************/
/********* Mixer *********/
/*************************/

PaError OpenStream(PaStream **mStream, PaDeviceIndex Device, double Rate, void* Sound, double &dLatency, PaStreamCallback Callback)
{
		PaStreamParameters outputParams;

		outputParams.device = Device;
		outputParams.channelCount = 2;
		outputParams.sampleFormat = paInt16;
		outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;

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

	void MixBuffer(char* Src, char* Dest, int Length, int Start, float Multiplier)
	{
		Src += Start;

		while (Length)
		{
			float A = ((*Src) * Multiplier + *Dest);

			*Dest = A;

			Dest++;
			Src++;
			Length--;
		}
	}

	int SizeAvailable;
	bool Threaded;

	uint32 PlayingVoices;

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

		if (UseWasapi)
		{
			OpenStream( &Stream, GetWasapiDevice(), 44100, (void*) this, Latency, Mix );
		}
		else
		{
			OpenStream( &Stream, Pa_GetDefaultOutputDevice(), 44100, (void*) this, Latency, Mix );
		}

		Pa_StartStream( Stream );
		ConstFactor = 1.0;
	}

	void Run()
	{
		int read = 1;
		char TempStream[BUFF_SIZE*sizeof(int16)];
		char TempSave[BUFF_SIZE*sizeof(int16)];

		do
		{
			SizeAvailable = PaUtil_GetRingBufferWriteAvailable(&RingBuf);

			PlayingVoices = 0;

			if (SizeAvailable > 0)
			{
				memset(TempStream, 0, sizeof(TempStream));
				memset(TempSave, 0, sizeof(TempSave));

				mut.lock();
				for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
				{
					if ((*i)->IsPlaying())
					{
						(*i)->Update();
						PlayingVoices++;
					}
				}
				mut.unlock();

				mut2.lock();
				for(std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
				{
					if ((*i)->IsPlaying())
						PlayingVoices++;
				}
				mut2.unlock();

				// PaUtil_WriteRingBuffer(&RingBuf, TempStream, SizeAvailable);
			}/*else
			{
				boost::mutex::scoped_lock lk(rbufmux);
				// Wait for it.
				ringbuffer_has_space.wait(lk);
				SizeAvailable = PaUtil_GetRingBufferWriteAvailable(&RingBuf);
			}*/
		} while (Threaded);
	}

	void AppendMusic(SoundStream* Stream)
	{
		mut.lock();
		Streams.push_back(Stream);
		mut.unlock();
	}

	void RemoveMusic(SoundStream *Stream)
	{
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
		double Voices = 0;
		memset(out, 0, samples * 2);

		mut.lock();
		for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			memset(ts, 0, sizeof(ts));
			if ((*i)->Read(ts, samples)) Voices++;
			MixBuffer((char*)ts, (char*)out, samples * 2, 0, VolumeMusic);
		}
		mut.unlock();

		// ringbuffer_has_space.notify_one();

		mut2.lock();
		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				memset(ts, 0, sizeof(ts));
				if ((*i)->Read(ts, samples)) Voices++;
				MixBuffer((char*)ts, (char*)out, samples * 2, 0, VolumeSFX);
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

		if (Index->type == paWASAPI)
			DefaultWasapiDevice = Index->defaultOutputDevice;
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
	PaError Err = Pa_Initialize();

	UseWasapi = (Configuration::GetConfigf("UseWasapi") != 0);
	UseThreadedDecoder = (Configuration::GetConfigf("UseThreadedDecoder") != 0);

	GetAudioInfo();

	Mixer = new PaMixer(UseThreadedDecoder);
	assert (Err == 0 && Mixer);
}

double GetDeviceLatency()
{
	return Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
}

void MixerAddStream(SoundStream *Sound)
{
	Mixer->AppendMusic(Sound);
}

void MixerRemoveStream(SoundStream* Sound)
{
	Mixer->RemoveMusic(Sound);
}

void MixerAddSample(SoundSample* Sample)
{
	Mixer->AddSound(Sample);
}

void MixerRemoveSample(SoundSample* Sample)
{
	Mixer->RemoveSound(Sample);
}

void MixerUpdate()
{
	if (!UseThreadedDecoder)
		Mixer->Run();
}

double MixerGetLatency()
{
	return Mixer->GetLatency();
}

double MixerGetFactor()
{
	return Mixer->GetFactor();
}