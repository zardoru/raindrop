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
	bool Open(const char* Filename) override;
	shared_ptr<SoundSample> GetFromIndex(int Index);
	void Seek(float Time) override;
	uint32 Read(short* buffer, size_t count) override;

	size_t GetLength() override; // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate() override; // Returns sampling rate of audio
	uint32 GetChannels() override; // Returns channels of audio
	bool IsValid() override;
	bool HasDataLeft() override;
	void SetPitch(double speed);
};