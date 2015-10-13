struct SF_INFO;

#include "Interruptible.h"

class AudioSourceOJM : public AudioDataSource, Interruptible
{
	static const int OJM_OGG = 1;
	static const int OJM_WAV = 2;

	struct 
	{
		int Enabled;
		void *Info;
		void* File;
	} TemporaryState;

	shared_ptr<SoundSample> Arr[2000];
	shared_ptr<std::ifstream> ifile;
	void parseM30();
	void parseOMC();

	double Speed;
public:
	AudioSourceOJM(Interruptible* Parent = nullptr);
	~AudioSourceOJM();
	bool Open(const char* Filename);
	shared_ptr<SoundSample> GetFromIndex(int Index);
	void Seek(float Time);
	uint32 Read(short* buffer, size_t count);

	size_t GetLength(); // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate(); // Returns sampling rate of audio
	uint32 GetChannels(); // Returns channels of audio
	bool IsValid();
	bool HasDataLeft();
	void SetPitch(double speed);
};