#include "Global.h"
#include "Audio.h"
#include <cstdio>
#include <cstring>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/thread/condition.hpp>

float VolumeSFX = 1.0f;
float VolumeMusic = 1.0f; 

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

	SeekTime = -1;
	runThread = 0;
	streamTime = playbackTime = 0;
	threadRunning = false;
	loop = false;
	thread = NULL;
}

VorbisStream::VorbisStream(const char* Filename, uint32 bufferSize)
{
	int32 retv = ov_fopen(Filename, &f);

	if (retv == 0)
	{
		info = ov_info(&f, -1);
		comment = ov_comment(&f, -1);
		BufSize = bufferSize;

		buffer = new char[sizeof(int16) * bufferSize];

		PaUtil_InitializeRingBuffer(&RingBuf, sizeof(int16), bufferSize, buffer);
		isOpen = true;
		UpdateBuffer(retv);
	}else
		isOpen = false;	

	SeekTime = -1;
	runThread = false;
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

	clearBuffer();
	ov_time_seek(&f, 0);
	SeekTime = -1;


	runThread = false;
}

void VorbisStream::clearBuffer()
{
	memset(buffer, 0, BufSize);
	memset(tbuf, 0, BufSize);
	PaUtil_InitializeRingBuffer(&RingBuf, sizeof(int16), BufSize, buffer);
}

bool VorbisStream::IsOpen() const
{
	return isOpen;
}

void VorbisStream::Start()
{
	runThread = 1;
}

void VorbisStream::UpdateBuffer(int32 &read)
{
	int32 sect;
	int32 count;
	int32 size;

	read = 0;

	if (!isOpen)
		return;

	count = PaUtil_GetRingBufferWriteAvailable(&RingBuf);
	size = count*sizeof(uint16);

	if (SeekTime >= 0)
	{
		ov_time_seek(&f, SeekTime);
		SeekTime = -1;
	}

	/* read from ogg vorbis file */
	while (read < size)
	{
		int32 res = ov_read(&f, (char*)tbuf+read, size - read, 0, 2, 1, &sect);
		if (res > 0)
			read += res;
		else if (res == 0)
		{
			if (loop)
			{
				ov_time_seek(&f, 0);
				continue;
			}
			else
			{
				Stop();
				return;
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

	runThread = 1;
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

int32 VorbisStream::readBuffer(void * out, uint32 length)
{
	size_t cnt;
	size_t toRead = length*info->channels;
	
	if (PaUtil_GetRingBufferReadAvailable(&RingBuf) < toRead || !runThread)
	{
		memset(out, 0, toRead);
		toRead = PaUtil_GetRingBufferReadAvailable(&RingBuf);
	}

	if (runThread)
	{
		cnt = PaUtil_ReadRingBuffer(&RingBuf, out, toRead);
		streamTime += (double)(cnt/info->channels) / (double)info->rate;
		playbackTime = streamTime - GetDeviceLatency();
	}

	return length;
}

double VorbisStream::getRate() const
{
	return info->rate;
}

int32 VorbisStream::getChannels() const
{
	return info->channels;
}


void VorbisStream::seek(double Time, bool accurate)
{
	SeekTime = Time;
	streamTime = playbackTime = Time;
}

void VorbisStream::setLoop(bool _loop)
{
	loop = _loop;
}

void VorbisStream::Stop()
{
	runThread = 0;
}

bool VorbisStream::IsStopped() const
{
	return runThread == 0;
}

double VorbisStream::GetPlaybackTime() const
{
	return playbackTime;
}

double VorbisStream::GetStreamedTime() const
{
	return streamTime;
}

static int32 StreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	bool cont = !((VorbisStream*)(userData))->readBuffer(output, frameCount);
	return cont;
}

/*********************************/
/********* Vorbis sample *********/
/*********************************/

VorbisSample::VorbisSample(const char* filename)
{
	valid = false;
	if (ov_fopen(filename, &f) == 0)
	{
		info = ov_info(&f, -1);
		comment = ov_comment(&f, -1);
		BufSize = ov_pcm_total(&f, -1)*sizeof(int16);
		buffer = new char[BufSize];

		Counter = BufSize;
		uint32 read = 0;
		int32 sect;
		while (read < BufSize)
		{
			int result = ov_read(&f, buffer + read, BufSize-read,0,2,1,&sect);

			if (result <= 0)
				break;
			read += result;
		}
		valid = true;
	}
}

VorbisSample::~VorbisSample()
{
	ov_clear(&f);
	delete buffer;
}

double VorbisSample::getRate()
{
	if (valid)
		return info->rate;
	else
		return 0;
}

int32 VorbisSample::getChannels()
{
	if (valid)
		return info->channels;
	else
		return 0;
}

void VorbisSample::Reset()
{
	Counter = 0;
}

int32 VorbisSample::readBuffer(void * out, uint32 length)
{
	if (valid)
	{
	length *= info->channels;
	if (Counter != BufSize)
	{		
		if(length > BufSize-Counter)
		{
			memset(out, 0, length);
			length = BufSize-Counter;
		}

		memcpy(out, buffer+Counter, length);

		Counter += length;
	}else
	{
		memset(out, 0, length);
	}

	return length;
	}else
		return 0;
}

/*************************/
/********* Mixer *********/
/*************************/

float waveshape_distort( float in ) {
  if(in <= -1.25f) {
    return -0.984375;
  } else if(in >= 1.25f) {
    return 0.984375;
  } else {    
    return 1.1f * in - 0.2f * in * in * in;
  }
}

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

class PaMixer
{
	PaStream* Stream;
	char *RingbufData;
	PaUtilRingBuffer RingBuf;

	std::vector<VorbisStream*> Streams;
	std::vector<VorbisSample*> Samples;

	void MixBuffer(char* Src, char* Dest, int Length, int Start, float Multiplier)
	{
		static float ConstFactor = 1.0f / sqrt(2.0f);
		Src += Start;
		while (Length)
		{
			// *Dest = (int)*Src + (int)*Dest - (((int)*Src) * ((int)*Dest) / 256);

			if (*Dest && *Src)
			{
				float A = (float)*Src / 255.0, B = (float)*Dest / 255.0;
				float mex = (A+B) * ConstFactor * Multiplier * 255;
				float ClipVal = A+B;


				*Dest = mex;
			}
			else
				*Dest += *Src;
				
			/*if (*Dest > 0 && *Src > 0)
			{
				*Dest = ((float)*Dest + (float)*Src) - (((float)*Dest) * ((float)*Src))/((float)255);
			}else if (*Dest < 0 && *Src < 0)
			{
				*Dest = ((float)*Dest + (float)*Src) - (((float)*Dest) * ((float)*Src))/(-255.0);
			}else
			{
				*Dest += *Src;
			}*/
			/*
			float A = (float)*Src / 256.0, B = (float)*Dest / 256.0;
			float mixed = (A+B)*0.8;
			if (mixed > 1.0f) mixed = 0.9f;
			if (mixed < -1.0f) mixed = -0.9f;
			
			*Dest = mixed*255;
			*/
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
				for(std::vector<VorbisStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
				{
					if ((*i)->runThread)
					{
						(*i)->UpdateBuffer(read);

						memset(TempSave, 0, sizeof(TempSave));

						(*i)->readBuffer(TempSave, SizeAvailable/2);
						MixBuffer(TempSave, TempStream, SizeAvailable*2, 0, VolumeMusic);
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

	void AppendMusic(VorbisStream* Stream)
	{
		mut.lock();
		Streams.push_back(Stream);
		mut.unlock();
	}

	void RemoveMusic(VorbisStream *Stream)
	{
		mut.lock();
		for(std::vector<VorbisStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
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

	void AddSound(VorbisSample* Sample)
	{
		mut2.lock();
		Samples.push_back(Sample);
		mut2.unlock();
	}

	void RemoveSound(VorbisSample* Sample)
	{
		mut2.lock();
		for(std::vector<VorbisSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			if ((*i) == Sample)
				i = Samples.erase(i);
		}
		mut2.unlock();
	}

	private:
		char ts[BUFF_SIZE*2];
	public:

	void CopyOut(char* out, int length)
	{
		if (Streams.size())
		for(std::vector<VorbisStream*>::iterator i = Streams.begin(); i != Streams.end(); i++)
		{
			if ((*i)->runThread && (*i)->streamTime > 0)
			{
				(*i)->playbackTime = (*i)->streamTime - GetDeviceLatency();
			}
		}

		ringbuffer_has_space.notify_one();
		PaUtil_ReadRingBuffer(&RingBuf, out, length*2);

		mut2.lock();
		for (std::vector<VorbisSample*>::iterator i = Samples.begin(); i != Samples.end(); i++)
		{
			memset(ts, 0, sizeof(ts));
			(*i)->readBuffer(ts, length*2);
			MixBuffer(ts, out, length*4, 0, VolumeSFX);
		}
		mut2.unlock();
	}
};

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
	PaMixer *Mix = (PaMixer*)userData;
	Mix->CopyOut((char*)output, frameCount);
	return 0;
}

/****************************/
/* PortAudio Stream Wrapper */
/****************************/

PaMixer *Mixer;

PaStreamWrapper::PaStreamWrapper(VorbisStream *Vs)
{
	outputParams.device = Pa_GetDefaultOutputDevice();
	outputParams.channelCount = Vs->getChannels();
	outputParams.sampleFormat = paInt16;
	outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
	outputParams.hostApiSpecificStreamInfo = NULL;
	Sound = Vs;
	mStream = NULL;
}

PaStreamWrapper::PaStreamWrapper(const char* filename)
{
	FILE * fp = fopen(filename, "rb");
	Sound = NULL;

	mStream = NULL;

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
	if (Sound)
		Pa_OpenStream(&mStream, NULL, &outputParams, Sound->getRate(), 0, 0, StreamCallback, (void*)Sound);
}

PaStreamWrapper::~PaStreamWrapper()
{
	if (mStream)
		Pa_CloseStream(mStream);
}

VorbisStream *PaStreamWrapper::GetStream()
{
	return Sound;
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

	if (IsValid())
	{
		Sound->loop = looping;

		// start filling the ring buffer
		if (stream)
			Sound->startStream();
		Pa_StartStream(mStream);
	}
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

String GetOggTitle(String file)
{
	OggVorbis_File f;
	String result = "";
	if (ov_fopen(file.c_str(), &f) == 0)
	{
		vorbis_comment *comment = ov_comment(&f, -1);

		for (int i = 0; i < comment->comments; i++)
		{
			std::vector<String> splitvec;
			std::string user_comment = comment->user_comments[i];
			boost::split(splitvec, user_comment, boost::is_any_of("="));
			if (splitvec[0] == "TITLE")
			{
				result = splitvec[1].c_str();
				break;
			}
		}

		ov_clear(&f);
	}

	return result;
}

void MixerAddStream(VorbisStream *Sound)
{
	Mixer->AppendMusic(Sound);
}

void MixerRemoveStream(VorbisStream* Sound)
{
	Mixer->RemoveMusic(Sound);
}

void MixerAddSample(VorbisSample * Sample)
{
	Mixer->AddSound(Sample);
}

void MixerUpdate()
{
	Mixer->Run();
}
