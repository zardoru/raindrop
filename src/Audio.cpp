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
float VolumeSFX = 1;
float VolumeMusic = 1; 
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
		wprintf(L"%s\n", Pa_GetErrorText(Err));
		wprintf(L"Device Selected %d\n", Device);
	}
#ifdef LINUX
	else
	{
		wprintf(L"Audio: Enabling real time scheduling\n");
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

	int SizeAvailable;
	bool Threaded, WaitForRingbufferSpace;

	boost::mutex mut, mut2, rbufmux;
	boost::condition ringbuffer_has_space;
public:
	PaMixer(bool StartThread)
	{
		RingbufData = new char[BUFF_SIZE*sizeof(int16)];

		WaitForRingbufferSpace = false;

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
		
		Latency = Pa_GetStreamInfo(Stream)->outputLatency;

		wprintf(L"AUDIO: Latency after opening stream = %f \n", Latency);

		ConstFactor = 1.0;
	}

	void Run()
	{
		do
		{

				WaitForRingbufferSpace = true;

				if (Threaded)
				{
					mut.lock();
				}


				for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
				{
					if ((*i)->IsPlaying())
					{
						(*i)->Update();
					}
				}

				if (Threaded)
				{
					boost::unique_lock<boost::mutex> lock (rbufmux);
					mut.unlock();

					while (WaitForRingbufferSpace)
					{
						ringbuffer_has_space.wait(lock);
					}
				}

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
		short ts[BUFF_SIZE*2];
		int tsF[BUFF_SIZE*2];
	public:

	void CopyOut(char* out, int samples)
	{
		int Voices = 0;
		int StreamVoices = 0;
		int count = samples;
		
		memset(out, 0, count * sizeof(short));
		memset(tsF, 0, sizeof(tsF));

		// mut.lock();
		for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				StreamVoices++;
				Voices++;
			}
		}

		// mut.unlock();

		mut2.lock();
		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				Voices++;
			}
		}

		double MixFactor = 1.0 / sqrt((double)Voices + 2);

		for(std::vector<SoundStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				memset(ts, 0, sizeof(ts));
				(*i)->Read(ts, samples);

				for (int i = 0; i < count; i++)
					tsF[i] += ts[i];
			}
		}

		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				memset(ts, 0, sizeof(ts));
				(*i)->Read(ts, samples);

				for (int i = 0; i < count; i++)
					tsF[i] += ts[i];
			}
		}

		for (int i = 0; i < count; i++)
		{
			tsF[i] *= MixFactor;
			((short*)out)[i] = tsF[i];
		}

		mut2.unlock();

		if (StreamVoices)
		{
			WaitForRingbufferSpace = false;
			ringbuffer_has_space.notify_one();
		}
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

	wprintf(L"AUDIO: The default API is %d\n", Pa_GetDefaultHostApi());

	for (PaHostApiIndex i = 0; i < ApiCount; i++)
	{
		const PaHostApiInfo* Index = Pa_GetHostApiInfo(i);
		wprintf(L"(%d) %s: %d (%d)\n", i, Utility::Widen(Index->name).c_str(), Index->defaultOutputDevice, Index->type);

#ifdef WIN32
		if (Index->type == paWASAPI)
			DefaultWasapiDevice = Index->defaultOutputDevice;
#endif
	}

	wprintf(L"\nAUDIO: The audio devices are\n");
	
	PaDeviceIndex DevCount = Pa_GetDeviceCount();
	for (PaDeviceIndex i = 0; i < DevCount; i++)
	{
		const PaDeviceInfo *Info = Pa_GetDeviceInfo(i);
		wprintf(L"(%d): %s\n", i, Utility::Widen(Info->name).c_str());
		wprintf(L"\thighLat: %f, lowLat: %f\n", Info->defaultHighOutputLatency, Info->defaultLowOutputLatency);
		wprintf(L"\tsampleRate: %f, hostApi: %d\n", Info->defaultSampleRate, Info->hostApi);
		wprintf(L"\tmaxchannels: %d\n", Info->maxOutputChannels);
	}
}

void InitAudio()
{
#ifndef NO_AUDIO
	PaError Err = Pa_Initialize();

	if (Err != 0) // Couldn't get audio, bail out
		return;

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
