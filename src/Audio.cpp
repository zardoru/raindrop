#include "Audio.h"
#include <cstdio>
#include <cstring>
#include <boost/thread.hpp>

/* Vorbis file stream */

VorbisStream::VorbisStream(FILE *fp, int bufferSize)
{
	int retv = ov_open(fp, &f, NULL, 0);

	info = ov_info(&f, -1);
	comment = ov_comment(&f, -1);

	buffer = new char[sizeof(short int) * BUFF_SIZE];

	PaUtil_InitializeRingBuffer(&RingBuf, sizeof(short int), BUFF_SIZE, buffer);
	runThread = true;
}

VorbisStream::~VorbisStream()
{
	delete buffer;
}

void VorbisStream::clearBuffer()
{
	memset(buffer, 0, BufSize);
}

void VorbisStream::UpdateBuffer(int &read)
{
	int sect;
	int count = PaUtil_GetRingBufferWriteAvailable(&RingBuf);
	int size = count*sizeof(short int);
	read = 0;

	/* read from ogg vorbis file */
	while (read < size)
	{
		int res = ov_read(&f, (char*)tbuf+read, size - read, 0, 2, 1, &sect);
		if (res)
			read += res;
		else if (res == 0)
		{
			if (loop)
				ov_time_seek(&f, 0);
			else
				runThread = 0;
			continue;
		}
		else 
		{
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
	int read = 1;
	while (read > 0 && runThread)
	{
		UpdateBuffer(read);
	}
}

int VorbisStream::readBuffer(void * out, int length,const PaStreamCallbackTimeInfo *timeInfo)
{
	char *outpt = (char*) out;

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

int VorbisStream::getChannels()
{
	return info->channels;
}


static int StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
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

PaStreamWrapper::PaStreamWrapper(char* filename)
{
	FILE * fp = fopen(filename, "rb");
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
}

PaStreamWrapper::~PaStreamWrapper()
{
	Pa_StopStream(mStream);
	delete Sound;
}

void PaStreamWrapper::Start(bool looping)
{
	Sound->loop = looping;

	// start filling the ring buffer
	boost::thread sound_upd (&VorbisStream::operator(), Sound);

	// fire up portaudio
	PaError Err = Pa_OpenStream(&mStream, NULL, &outputParams, Sound->getRate(), 0, 0, StreamCallback, (void*)Sound);
	Pa_StartStream(mStream);
	return;
}

void InitAudio()
{
	PaError Err = Pa_Initialize();
#ifdef DEBUG
	if (Err != 0)
#ifdef WIN32
		__asm int 3;
#else
		asm("int 3");
#endif // WIN32
#endif // DEBUG
}