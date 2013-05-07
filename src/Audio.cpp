#include "Global.h"
#include "Audio.h"
#include <cstdio>
#include <cstring>
#include <boost/thread.hpp>

/* Vorbis file stream */

VorbisStream::VorbisStream(FILE *fp, uint32 bufferSize)
{
	int32 retv = ov_open(fp, &f, NULL, 0);

	if (retv == 0)
	{
		info = ov_info(&f, -1);
		comment = ov_comment(&f, -1);
		BufSize = bufferSize;

		buffer = new char[sizeof(int16) * bufferSize];

		PaUtil_InitializeRingBuffer(&RingBuf, sizeof(int16), bufferSize, buffer);
		isOpen = true;
	}else
		isOpen = false;	


	streamTime = playbackTime = 0;
	threadRunning = false;
	loop = false;
	thread = NULL;
}

VorbisStream::~VorbisStream()
{
	ov_clear(&f);
	delete buffer;
}

void VorbisStream::startStream()
{
	if (!thread)
	{
		runThread = true;
		thread = new boost::thread(&VorbisStream::operator(), this);
	}
}

void VorbisStream::stopStream()
{
	if (thread)
	{
		runThread = false;
		thread->join();
		delete thread;
		thread = NULL;
	}
}

void VorbisStream::clearBuffer()
{
	memset(buffer, 0, BufSize);
}

bool VorbisStream::IsOpen()
{
	return isOpen;
}


void VorbisStream::UpdateBuffer(int32 &read)
{
	int32 sect;
	int32 count = PaUtil_GetRingBufferWriteAvailable(&RingBuf);
	int32 size = count*sizeof(uint16);
	read = 0;

	if (SeekTime)
	{
		ov_time_seek(&f, SeekTime);
		SeekTime = 0;
	}

	/* read from ogg vorbis file */
	while (read < size)
	{
		int32 res = ov_read(&f, (char*)tbuf+read, size - read, 0, 2, 1, &sect);
		if (res)
			read += res;
		else if (res == 0)
		{
			if (loop)
				ov_time_seek(&f, 0);
			else
			{
				clearBuffer();
				runThread = 0;
			}
			break;
		}
		else 
		{
			clearBuffer();
			runThread = 0;
			return;
		}
	}

	/* fill into ring buffer */
	PaUtil_WriteRingBuffer(&RingBuf, tbuf, count);

	if (!size)
		read = 1;
}

void VorbisStream::operator () ()
{
	int32 read = 1;

	threadRunning = true;

	while (read > 0 && runThread)
	{
		UpdateBuffer(read);
	}

	threadRunning = false;
}

int32 VorbisStream::readBuffer(void * out, uint32 length,const PaStreamCallbackTimeInfo *timeInfo)
{
	char *outpt = (char*) out;

	playbackTime += (double)length / (double)info->rate;

	PaUtil_ReadRingBuffer(&RingBuf, out, length*info->channels);

	if (runThread)
		return 0;
	else
		return 1;
}

double VorbisStream::getRate()
{
	return info->rate;
}

int32 VorbisStream::getChannels()
{
	return info->channels;
}


void VorbisStream::seek(double Time, bool accurate)
{
	SeekTime = Time;
}

static int32 StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	return ((VorbisStream*)(userData))->readBuffer(output, frameCount, timeInfo);
}

/* PortAudio Stream Wrapper */


PaStreamWrapper::PaStreamWrapper(VorbisStream *Vs)
{
	outputParams.device = Pa_GetDefaultOutputDevice();
	outputParams.channelCount = Vs->getChannels();
	outputParams.sampleFormat = paInt16;
	outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
	outputParams.hostApiSpecificStreamInfo = NULL;
	Sound = Vs;
}

PaStreamWrapper::PaStreamWrapper(const char* filename)
{
	FILE * fp = fopen(filename, "rb");
	Sound = NULL;
	if (fp)
	{
		VorbisStream *Vs = new VorbisStream(fp);
		outputParams.device = Pa_GetDefaultOutputDevice();
		outputParams.channelCount = Vs->getChannels();
		outputParams.sampleFormat = paInt16;
		outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
		outputParams.hostApiSpecificStreamInfo = NULL;
		Sound = Vs;
	}

	// fire up portaudio
	Pa_OpenStream(&mStream, NULL, &outputParams, Sound->getRate(), 0, 0, StreamCallback, (void*)Sound);
}

PaStreamWrapper::~PaStreamWrapper()
{
	if (mStream)
		Pa_CloseStream(mStream);
}

void PaStreamWrapper::Stop()
{
	if (mStream)
		Pa_StopStream(mStream);
	if (Sound)
	{
		Sound->stopStream();
	}
}

void PaStreamWrapper::Start(bool looping, bool stream)
{
	Sound->loop = looping;

	if (IsValid())
	{
		// start filling the ring buffer
		if (stream)
			Sound->startStream();
		Pa_StartStream(mStream);
	}

	return;
}

void PaStreamWrapper::Seek(double Time, bool Accurate, bool RestartStream)
{
	// this stops the sound streaming thread and flushes the ring buffer
	Sound->seek(Time, Accurate);
}

bool PaStreamWrapper::IsValid()
{
	return mStream && Sound && Sound->IsOpen();
}

double PaStreamWrapper::GetPlaybackTime()
{
	if (IsValid())
		return Sound->playbackTime;
	else
		return 0;
}

bool PaStreamWrapper::IsStopped()
{
	return !Sound->runThread;
}

void InitAudio()
{
	PaError Err = Pa_Initialize();
#ifdef DEBUG
	if (Err != 0)
		Utility::DebugBreak();
#endif // DEBUG
}

double GetDeviceLatency()
{
	return Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
}