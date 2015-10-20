#include <math.h>

#include "Global.h"
#include "Audio.h"
#include "Configuration.h"
#include "Logging.h"

#include <portaudio.h>
#include <pa_ringbuffer.h>

#ifdef WIN32
#include <pa_win_wasapi.h>
#include <pa_win_ds.h>
#endif

#ifdef LINUX
#include <pa_linux_alsa.h>
#endif

#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

float VolumeSFX = 1;
float VolumeMusic = 1; 
bool UseThreadedDecoder = false;
bool Normalize;

#ifdef WIN32
bool UseWasapi = false;
PaDeviceIndex DefaultWasapiDevice;
PaDeviceIndex DefaultDSDevice;
#endif

/*************************/
/********* Mixer *********/
/*************************/

PaError OpenStream(PaStream **mStream, PaDeviceIndex Device, double Rate, void* Sound, double &dLatency, PaStreamCallback Callback)
{
		PaStreamParameters outputParams;

		outputParams.device = Device;
		outputParams.channelCount = 2;
		outputParams.sampleFormat = paFloat32;

		if (!Configuration::GetConfigf("UseHighLatency", "Audio"))
			outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
		else
			outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
			

#ifndef WIN32

		outputParams.hostApiSpecificStreamInfo = NULL;

#else
		PaWasapiStreamInfo StreamInfo;
		PaWinDirectSoundStreamInfo DSStreamInfo;
		if (UseWasapi)
		{
			outputParams.hostApiSpecificStreamInfo = &StreamInfo;
			StreamInfo.hostApiType = paWASAPI;
			StreamInfo.size = sizeof(PaWasapiStreamInfo);
			StreamInfo.version = 1;
			if (!Configuration::GetConfigf("WasapiDontUseExclusiveMode", "Audio"))
			{
				StreamInfo.threadPriority = eThreadPriorityProAudio;
				StreamInfo.flags = paWinWasapiExclusive;
			}
			else
			{
				StreamInfo.threadPriority = eThreadPriorityAudio;
				StreamInfo.flags = 0; 
			}
	
			StreamInfo.hostProcessorOutput = NULL;
			StreamInfo.hostProcessorInput = NULL;
		}else
		{
			outputParams.hostApiSpecificStreamInfo = &DSStreamInfo;

			DSStreamInfo.size = sizeof(PaWinDirectSoundStreamInfo);
			DSStreamInfo.hostApiType = paDirectSound;
			DSStreamInfo.version = 2;
			DSStreamInfo.flags = 0;
		}
#endif

	dLatency = outputParams.suggestedLatency;

	// fire up portaudio
	PaError Err = Pa_OpenStream(mStream, NULL, &outputParams, Rate, 0, paClipOff, Callback, (void*)Sound);

	if (Err)
	{
		Log::Logf("%ls\n", Utility::Widen(Pa_GetErrorText(Err)).c_str());
		Log::Logf("Device Selected %d\n", Device);
	}
#ifdef LINUX
	else
	{
		Log::Logf("Audio: Enabling real time scheduling\n");
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
	bool Threaded;
	atomic<bool> WaitForRingbufferSpace;

	mutex mut, mut2, rbufmux;
	condition_variable ringbuffer_has_space;

	PaMixer(){};
public:
	

	static PaMixer &GetInstance()
	{
		static PaMixer *Mixer = new PaMixer;
		return *Mixer;
	}

	void Initialize(bool StartThread)
	{
		RingbufData = new char[BUFF_SIZE*sizeof(float)];

		WaitForRingbufferSpace = false;

		PaUtil_InitializeRingBuffer(&RingBuf, sizeof(float), BUFF_SIZE, RingbufData);

		Threaded = StartThread;
		Stream = nullptr;

		if (StartThread)
		{
			thread (&PaMixer::Run, this).detach();
		}
#ifdef WIN32
		if (UseWasapi)
		{
			OpenStream( &Stream, GetWasapiDevice(), 44100, (void*) this, Latency, Mix );

			if (!Stream)
			{
				// This was a Wasapi problem. Retry without it.
				Log::Logf("Problem initializing WASAPI. Falling back to default API.");
				UseWasapi = false;
				OpenStream( &Stream, Pa_GetDefaultOutputDevice(), 44100, (void*) this, Latency, Mix );
			}

		}
		else
		{
			OpenStream( &Stream, DefaultDSDevice, 44100, (void*) this, Latency, Mix );
		}
#else
			OpenStream( &Stream, Pa_GetDefaultOutputDevice(), 44100, (void*) this, Latency, Mix );

#endif

		if (Stream)
		{
			Pa_StartStream( Stream );
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			Latency = Pa_GetStreamInfo(Stream)->outputLatency;
			Log::Logf("AUDIO: Latency after opening stream = %f \n", Latency);
		}

		ConstFactor = 1.0;
	}

	void Run()
	{
		do
		{
				WaitForRingbufferSpace = true;

				if (Threaded)
					mut.lock();


				for(auto i = Streams.begin(); i != Streams.end(); ++i)
				{
					if ((*i)->IsPlaying())
					{
						(*i)->Update();
					}
				}

				if (Threaded)
				{
					unique_lock<mutex> lock (rbufmux);
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
		mut.lock();
		Streams.push_back(Stream);
		mut.unlock();
	}

	void RemoveMusic(SoundStream *Stream)
	{
		mut.lock();
		for(auto i = Streams.begin(); i != Streams.end();)
		{
			if ((*i) == Stream)
			{
				i = Streams.erase(i);
				if (i != Streams.end())
					continue;
				else
					break;
			}

			++i;
		}
		mut.unlock();
	}

	void AddSound(SoundSample* Sample)
	{
		mut.lock();
		Samples.push_back(Sample);
		mut.unlock();
	}

	void RemoveSound(SoundSample* Sample)
	{
		mut.lock();
		for(auto i = Samples.begin(); i != Samples.end(); )
		{
			if ((*i) == Sample)
			{
				i = Samples.erase(i);
				
				if (i == Samples.end())
					break;
				else continue;
			}

			++i;
		}

		mut.unlock();
	}

	private:
		float ts[BUFF_SIZE*2];
		float tsF[BUFF_SIZE*2];

	public:

	void CopyOut(float * out, int samples)
	{
		int count = samples;

		memset(out, 0, samples * sizeof(float));

		bool streaming = false;
		mut.lock();
		for(auto i = Streams.begin(); i != Streams.end(); ++i)
		{
			size_t read = (*i)->Read(ts, samples);

			streaming |= (*i)->IsPlaying();
			for (size_t k = 0; k < read; k++)
				out[k] += ts[k];
		}

		for (auto i = Samples.begin(); i != Samples.end(); ++i)
		{
			size_t read = (*i)->Read(ts, samples);

			for (size_t k = 0; k < read; k++)
				out[k] += ts[k];
		}
		mut.unlock();

		if (Normalize)
		{
			float peak = 1.0;
			for (int i = 0; i < count; i++)
				peak = max(peak, abs(out[i]));

			for (int i = 0; i < count; i++)
				out[i] /= peak;
		}
		
		if (streaming)
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
	Mix->CopyOut((float*)output, frameCount * 2);
	return 0;
}

/*************************/
/********** API **********/
/*************************/

void GetAudioInfo()
{
	PaHostApiIndex ApiCount = Pa_GetHostApiCount();

	Log::Logf("AUDIO: The default API is %d\n", Pa_GetDefaultHostApi());

	for (PaHostApiIndex i = 0; i < ApiCount; i++)
	{
		const PaHostApiInfo* Index = Pa_GetHostApiInfo(i);
		Log::Logf("(%d) %s: %d (%d)\n", i, Index->name, Index->defaultOutputDevice, Index->type);

#ifdef WIN32
		if (Index->type == paWASAPI)
			DefaultWasapiDevice = Index->defaultOutputDevice;
		else if (Index->type == paDirectSound)
			DefaultDSDevice = Index->defaultOutputDevice;
#endif
	}

	Log::Logf("\nAUDIO: The audio devices are\n");
	
	PaDeviceIndex DevCount = Pa_GetDeviceCount();
	for (PaDeviceIndex i = 0; i < DevCount; i++)
	{
		const PaDeviceInfo *Info = Pa_GetDeviceInfo(i);
		Log::Logf("(%d): %s\n", i, Info->name);
		Log::Logf("\thighLat: %f, lowLat: %f\n", Info->defaultHighOutputLatency, Info->defaultLowOutputLatency);
		Log::Logf("\tsampleRate: %f, hostApi: %d\n", Info->defaultSampleRate, Info->hostApi);
		Log::Logf("\tmaxchannels: %d\n", Info->maxOutputChannels);
	}
}


void InitAudio()
{
#ifndef NO_AUDIO
	PaError Err = Pa_Initialize();

	if (Err != 0) // Couldn't get audio, bail out
		return;

#ifdef WIN32
	UseWasapi = (Configuration::GetConfigf("UseWasapi", "Audio") != 0);
#endif

	Normalize = (Configuration::GetConfigf("Normalize", "Audio") != 0);

	UseThreadedDecoder = (Configuration::GetConfigf("UseThreadedDecoder", "Audio") != 0);

	GetAudioInfo();

	PaMixer::GetInstance().Initialize(UseThreadedDecoder);
	assert (Err == 0);
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
	PaMixer::GetInstance().AppendMusic(Sound);
#endif
}

void MixerRemoveStream(SoundStream* Sound)
{
#ifndef NO_AUDIO
	PaMixer::GetInstance().RemoveMusic(Sound);
#endif
}

void MixerAddSample(SoundSample* Sample)
{
#ifndef NO_AUDIO
	PaMixer::GetInstance().AddSound(Sample);
#endif
}

void MixerRemoveSample(SoundSample* Sample)
{
#ifndef NO_AUDIO
	PaMixer::GetInstance().RemoveSound(Sample);
#endif
}

void MixerUpdate()
{
#ifndef NO_AUDIO
	if (!UseThreadedDecoder)
		PaMixer::GetInstance().Run();
#endif
}

double MixerGetLatency()
{
#ifndef NO_AUDIO
	return PaMixer::GetInstance().GetLatency();
#else
	return 0;
#endif
}

double MixerGetFactor()
{
#ifndef NO_AUDIO
	return PaMixer::GetInstance().GetFactor();
#else
	return 0;
#endif
}
