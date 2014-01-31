#include "Global.h"
#include "Audio.h"
#include <cstdio>
#include <cstring>

#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
float VolumeSFX = 1.0f;
float VolumeMusic = 1.0f; 

/* Vorbis file stream */

static int32 StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	bool cont = !((SoundStream*)(userData))->Read(output, frameCount * 2);
	return cont;
}
/*************************/
/********* Mixer *********/
/*************************/

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

class PaMixer
{
	PaStream* Stream;
	char *RingbufData;
	PaUtilRingBuffer RingBuf;

	std::vector<SoundStream*> Streams;
	std::vector<SoundSample*> Samples;

	void MixBuffer(char* Src, char* Dest, int Length, int Start, float Multiplier)
	{
		static const float ConstFactor = 1.0f / sqrt(2.0f);
		Src += Start;

		while (Length)
		{
			// *Dest = (int)*Src + (int)*Dest - (((int)*Src) * ((int)*Dest) / 256);

			if (*Dest && *Src)
			{
				float A = (float)*Src / 255.0, B = (float)*Dest / 255.0;
				float mex = (A+B) * ConstFactor * Multiplier * 255;

				*Dest = mex;
			}
			else
				*Dest += *Src;

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
		PaUtil_InitializeRingBuffer(&RingBuf, 2, BUFF_SIZE, RingbufData);

		Threaded = StartThread;

		if (StartThread)
		{
			boost::thread (&PaMixer::Run, this);
		}

		PaStreamParameters outputParams;
		outputParams.device = Pa_GetDefaultOutputDevice();
		outputParams.channelCount = 2;
		outputParams.sampleFormat = paInt16;
		outputParams.suggestedLatency = GetDeviceLatency();
		outputParams.hostApiSpecificStreamInfo = NULL;

		Pa_OpenStream(&Stream, NULL, &outputParams, 44100, 0, 0, Mix, (void*)this);
		Pa_StartStream(Stream);
	}

	void Run()
	{
		int read = 1;
		char TempStream[BUFF_SIZE*sizeof(int16)];
		char TempSave[BUFF_SIZE*sizeof(int16)];

		SizeAvailable = PaUtil_GetRingBufferWriteAvailable(&RingBuf);

		do
		{
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

						memset(TempSave, 0, sizeof(TempSave));

						(*i)->Read(TempSave, SizeAvailable);
						MixBuffer(TempSave, TempStream, SizeAvailable * 2, 0, VolumeMusic);
					}
				}
				mut.unlock();

				PaUtil_WriteRingBuffer(&RingBuf, TempStream, SizeAvailable);
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
				i = Samples.erase(i);
		}
		mut2.unlock();
	}

	private:
		char ts[BUFF_SIZE*2];
	public:

	void CopyOut(char* out, int samples)
	{
		PaUtil_ReadRingBuffer(&RingBuf, out, samples);
		ringbuffer_has_space.notify_one();

		mut2.lock();
		for (std::vector<SoundSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i)->IsPlaying())
			{
				memset(ts, 0, sizeof(ts));
				(*i)->Read(ts, samples);
				MixBuffer(ts, out, samples * 2, 0, VolumeSFX);
			}
		}
		mut2.unlock();
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
	outputParams.device = Pa_GetDefaultOutputDevice();
	outputParams.channelCount = Vs->GetChannels();
	outputParams.sampleFormat = paInt16;
	outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
	outputParams.hostApiSpecificStreamInfo = NULL;
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
		outputParams.device = Pa_GetDefaultOutputDevice();
		outputParams.channelCount = Vs->GetChannels();
		outputParams.sampleFormat = paInt16;
		outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
		outputParams.hostApiSpecificStreamInfo = NULL;
		Sound = Vs;
	}else
		delete Vs;

	// fire up portaudio
	if (Sound)
		Pa_OpenStream(&mStream, NULL, &outputParams, Sound->GetRate(), 0, 0, StreamCallback, (void*)Sound);
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

void PaStreamWrapper::Start(bool looping)
{
	if (IsValid())
	{
		Sound->SetLoop(looping);

		// start filling the ring buffer
		Sound->Play();
		Pa_StartStream(mStream);
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

void InitAudio()
{
	PaError Err = Pa_Initialize();
	Mixer = new PaMixer(false);
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

void MixerUpdate()
{
	Mixer->Run();
}
