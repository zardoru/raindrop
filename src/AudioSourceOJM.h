struct SF_INFO;

class AudioSourceOJM : public AudioDataSource
{
	struct 
	{
		int Enabled;
		SF_INFO *Info;
		void* File;
	} TemporaryState;

	SoundSample* Arr[2000];
	std::ifstream *ifile;
	void parseM30();
	void parseOMC();

public:
	AudioSourceOJM();
	~AudioSourceOJM();
	bool Open(const char* Filename);
	SoundSample* GetFromIndex(int Index);
	void Seek(float Time);
	uint32 Read(short* buffer, size_t count);

	size_t GetLength(); // Always returns total samples. Frames = Length/Channels.
	uint32 GetRate(); // Returns sampling rate of audio
	uint32 GetChannels(); // Returns channels of audio
	bool IsValid();
	bool HasDataLeft();
};