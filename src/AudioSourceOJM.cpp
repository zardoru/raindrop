#include <cstdio>
#include <map>
#include <sndfile.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <fstream>

#include "Global.h"
#include "Logging.h"
#include "Audio.h"
#include "AudioSourceOJM.h"


// Loader for OJM containers. Requires libsndfile. Based off the documentation at
// http://open2jam.wordpress.com/the-ojm-documentation/
// A lot of the code is just a carbon copy of OJMDumper, just C++.

/*
	For whoever decides to dig this code:
	While yes, it's mostly a carbon copy of OJMDumper, the additions here load the wav/ogg files using
	the libsndfile and vorbisfile libraries; thus a memory IO interface is first defined
	then used when the bits specific to raindrop's loading code happens.

	Furthermore, it does have of the bugfixes that open2jam later introduces instead of merely just using ojmdumper's code.
*/

struct M30Header
{
	int32 file_format_version;
	int32 encryption_flag;
	int32 sample_count;
	int32 sample_offset;
	int32 payload_size;
	int32 padding;
};



struct M30Entry
{
	char  sample_name[32];
	int32 sample_size;
	int16 codec_code;
	int16 codec_code2;
	int32 music_flag;
	int16 ref;
	int16 unk_zero;
	int32 pcm_samples;
};

struct OMC_header {
	int32 unk;
	int32 wav_start;
	int32 ogg_start;
	int32 fsize;
};

struct OMC_WAV_header {
	char sample_name[32];
	int16 audio_format;
	int16 num_channels;
	int32 sample_rate;
	int32 bit_rate;
	int16 block_align;
	int16 bits_per_sample;
	int32 unk_data;
	int32 chunk_size;
};

struct OMC_OGG_header {
	char sample_name[32];
	int32 sample_size;
};

struct SFM30
{
	size_t DataLength;
	size_t Offset;
	vector<char> Buffer;

	SFM30()
	{
		DataLength = 0; Offset = 0;
	}
};

sf_count_t getFileLenM30(void* p)
{
	auto state = static_cast<SFM30*>(p);
	return state->DataLength;
}

sf_count_t seekM30(sf_count_t offs, int whence, void* p)
{
	auto state = static_cast<SFM30*>(p);

	switch (whence)
	{
	case SEEK_CUR:
		state->Offset += offs;
		break;
	case SEEK_END:
		state->Offset = state->DataLength;
		break;
	case SEEK_SET:
		state->Offset = offs;
		break;
	}

	return state->Offset;
}

sf_count_t readM30(void* ptr, sf_count_t count, void* p)
{
	auto state = static_cast<SFM30*>(p);
	auto toRead = min(size_t(count), size_t(state->DataLength - state->Offset));

	if (state->Offset >= state->DataLength)
		return 0;
	else
	{
		memcpy(ptr, &state->Buffer[0] + state->Offset, toRead);
		state->Offset += toRead;
	}

	return toRead;
}

sf_count_t tellM30(void* p)
{
	auto state = static_cast<SFM30*>(p);
	return min(state->Offset, state->DataLength);
}

size_t readM30OGG(void* ptr, size_t size, size_t nmemb, void* p)
{
	auto state = static_cast<SFM30*>(p);
	int toRead = min(unsigned int(size*nmemb), unsigned int(state->DataLength - state->Offset));

	if (state->Offset >= state->DataLength)
		return 0;
	else
	{
		memcpy(ptr, &state->Buffer[0] + state->Offset, toRead);
		state->Offset += toRead;
	}

	return toRead;
}

int seekM30OGG(void* p, ogg_int64_t offs, int whence)
{
	auto state = static_cast<SFM30*>(p);

	switch (whence)
	{
	case SEEK_CUR:
		state->Offset += offs;
		break;
	case SEEK_END:
		state->Offset = state->DataLength;
		break;
	case SEEK_SET:
		state->Offset = offs;
		break;
	}

	return state->Offset;
}

long tellM30OGG(void* p)
{
	auto state = static_cast<SFM30*>(p);
	return state->Offset;
}


SF_VIRTUAL_IO M30Interface = {
	getFileLenM30,
	seekM30,
	readM30,
	NULL,
	tellM30
};

ov_callbacks M30InterfaceOgg = {
	readM30OGG,
	seekM30OGG,
	NULL,
	tellM30OGG
};

enum OJMContainerKind
{
	Undefined = -1,
	M30,
	OMC
};

char REARRANGE_TABLE[] = {
	0x10, 0x0E, 0x02, 0x09, 0x04, 0x00, 0x07, 0x01,
		0x06, 0x08, 0x0F, 0x0A, 0x05, 0x0C, 0x03, 0x0D,
		0x0B, 0x07, 0x02, 0x0A, 0x0B, 0x03, 0x05, 0x0D,
		0x08, 0x04, 0x00, 0x0C, 0x06, 0x0F, 0x0E, 0x10,
		0x01, 0x09, 0x0C, 0x0D, 0x03, 0x00, 0x06, 0x09,
		0x0A, 0x01, 0x07, 0x08, 0x10, 0x02, 0x0B, 0x0E,
		0x04, 0x0F, 0x05, 0x08, 0x03, 0x04, 0x0D, 0x06,
		0x05, 0x0B, 0x10, 0x02, 0x0C, 0x07, 0x09, 0x0A,
		0x0F, 0x0E, 0x00, 0x01, 0x0F, 0x02, 0x0C, 0x0D,
		0x00, 0x04, 0x01, 0x05, 0x07, 0x03, 0x09, 0x10,
		0x06, 0x0B, 0x0A, 0x08, 0x0E, 0x00, 0x04, 0x0B,
		0x10, 0x0F, 0x0D, 0x0C, 0x06, 0x05, 0x07, 0x01,
		0x02, 0x03, 0x08, 0x09, 0x0A, 0x0E, 0x03, 0x10,
		0x08, 0x07, 0x06, 0x09, 0x0E, 0x0D, 0x00, 0x0A,
		0x0B, 0x04, 0x05, 0x0C, 0x02, 0x01, 0x0F, 0x04,
		0x0E, 0x10, 0x0F, 0x05, 0x08, 0x07, 0x0B, 0x00,
		0x01, 0x06, 0x02, 0x0C, 0x09, 0x03, 0x0A, 0x0D,
		0x06, 0x0D, 0x0E, 0x07, 0x10, 0x0A, 0x0B, 0x00,
		0x01, 0x0C, 0x0F, 0x02, 0x03, 0x08, 0x09, 0x04,
		0x05, 0x0A, 0x0C, 0x00, 0x08, 0x09, 0x0D, 0x03,
		0x04, 0x05, 0x10, 0x0E, 0x0F, 0x01, 0x02, 0x0B,
		0x06, 0x07, 0x05, 0x06, 0x0C, 0x04, 0x0D, 0x0F,
		0x07, 0x0E, 0x08, 0x01, 0x09, 0x02, 0x10, 0x0A,
		0x0B, 0x00, 0x03, 0x0B, 0x0F, 0x04, 0x0E, 0x03,
		0x01, 0x00, 0x02, 0x0D, 0x0C, 0x06, 0x07, 0x05,
		0x10, 0x09, 0x08, 0x0A, 0x03, 0x02, 0x01, 0x00,
		0x04, 0x0C, 0x0D, 0x0B, 0x10, 0x05, 0x06, 0x0F,
		0x0E, 0x07, 0x09, 0x0A, 0x08, 0x09, 0x0A, 0x00,
		0x07, 0x08, 0x06, 0x10, 0x03, 0x04, 0x01, 0x02,
		0x05, 0x0B, 0x0E, 0x0F, 0x0D, 0x0C, 0x0A, 0x06,
		0x09, 0x0C, 0x0B, 0x10, 0x07, 0x08, 0x00, 0x0F,
		0x03, 0x01, 0x02, 0x05, 0x0D, 0x0E, 0x04, 0x0D,
		0x00, 0x01, 0x0E, 0x02, 0x03, 0x08, 0x0B, 0x07,
		0x0C, 0x09, 0x05, 0x0A, 0x0F, 0x04, 0x06, 0x10,
		0x01, 0x0E, 0x02, 0x03, 0x0D, 0x0B, 0x07, 0x00,
		0x08, 0x0C, 0x09, 0x06, 0x0F, 0x10, 0x05, 0x0A,
		0x04, 0x00};


// AZ: Let me conserve the comment from the original source code for ojmdumper.
/**
* fuck the person who invented this, FUCK YOU!... but with love =$
*/
void omc_rearrange(char* buf_io, size_t len)
{
	int key = ((len % 17) << 4) + (len % 17);
	int block_size = len / 17;
	vector<char> buf_encoded (len);
	memcpy(&buf_encoded[0], buf_io, len);

	for (int block = 0; block < 17; block++)
	{
		int block_start_encoded = block_size * block;	// Where is the start of the enconded block
		int block_start_plain = block_size * REARRANGE_TABLE[key];	// Where the final plain block will be
		memcpy(buf_io + block_start_plain, (&buf_encoded[0]) + block_start_encoded, block_size);

		key++;
	}
}

void omc_xor(char* buf, size_t len, int &acc_keybyte, int &acc_counter)
{
	int tmp;
	char this_byte = 0;

	for (size_t i = 0; i < len; i++)
	{
		tmp = this_byte = buf[i];

		if (((acc_keybyte << acc_counter) & 0x80) != 0)
			this_byte = ~this_byte;

		buf[i] = this_byte;
		acc_counter++;
		if (acc_counter > 7)
		{
			acc_counter = 0;
			acc_keybyte = tmp;
		}
	}
}

void NamiXOR(char* buffer, size_t length)
{
	char NAMI[] = { 0x6E, 0x61, 0x6D, 0x69 };
	for (size_t i = 0; i + 3 < length; i += 4)
	{
		buffer[i] ^= NAMI[0];
		buffer[i+1] ^= NAMI[1];
		buffer[i+2] ^= NAMI[2];
		buffer[i+3] ^= NAMI[3];
	}
}

void F412XOR(char* buffer, size_t length)
{
	char F412[] = { 0x30, 0x34, 0x31, 0x32 };
	for (size_t i = 0; i + 3 < length; i += 4)
	{
		buffer[i] ^= F412[0];
		buffer[i + 1] ^= F412[1];
		buffer[i + 2] ^= F412[2];
		buffer[i + 3] ^= F412[3];
	}
}

AudioSourceOJM::AudioSourceOJM(Interruptible* parent) : Interruptible(parent)
{
	TemporaryState.Enabled = false;
	Speed = 1;
}

AudioSourceOJM::~AudioSourceOJM()
{
}

OJMContainerKind GetContainerKind(const char* sig)
{
	if (!strcmp(sig, "M30"))
		return M30;
	else if (!strcmp(sig, "OMC") || !strcmp(sig, "OJM"))
		return OMC;

	return Undefined;
}

void AudioSourceOJM::parseM30()
{
	M30Header Head;
	size_t sizeLeft;
	ifile->read(reinterpret_cast<char*>(&Head), sizeof(M30Header));

	vector<char> Buffer(Head.payload_size);
	sizeLeft = Head.payload_size;

	for (int i = 0; i < Head.sample_count; i++)
	{
		if (sizeLeft < 52)
			break; // wrong number of samples

		M30Entry Entry;
		ifile->read(reinterpret_cast<char*>(&Entry), sizeof(M30Entry));
		sizeLeft -= sizeof(M30Entry);

		sizeLeft -= Entry.sample_size;

		int OJMIndex = Entry.ref;
		if (Entry.codec_code == 0)
			OJMIndex += 1000;
		else if (Entry.codec_code != 5) continue; // Unknown sample id type.

		vector<char> SampleData (Entry.sample_size);
		ifile->read(&SampleData[0], Entry.sample_size);
		
		if (Head.encryption_flag & 16)
			NamiXOR(&SampleData[0], Entry.sample_size);
		else if (Head.encryption_flag & 32)
			F412XOR(&SampleData[0], Entry.sample_size);

		// Sample data is done. Now the bits that are specific to raindrop..
		auto NewSample = make_shared<SoundSample>();

		SFM30 ToLoad;
		ToLoad.Buffer = move(SampleData);
		ToLoad.DataLength = Entry.sample_size;
		
		OggVorbis_File vf;

		ov_open_callbacks(&ToLoad, &vf, nullptr, 0, M30InterfaceOgg);
		TemporaryState.File = &vf;
		TemporaryState.Info = vf.vi;

		if (vf.vi)
		{
			TemporaryState.Enabled = OJM_OGG;
			NewSample->SetPitch(Speed);
			NewSample->Open(this);
			TemporaryState.Enabled = 0;
		}
		
		ov_clear(&vf);
		
		Arr[OJMIndex] = NewSample;
	}
}

void AudioSourceOJM::parseOMC()
{
	OMC_header Head;
	int acc_keybyte = 0xFF;
	int acc_counter = 0;
	int Offset = 20;
	int SampleID = 0;
	ifile->read(reinterpret_cast<char*>(&Head), sizeof(OMC_header));


	// Parse WAV data first
	while (Offset < Head.ogg_start)
	{
		CheckInterruption();

		OMC_WAV_header WavHead;
		ifile->read(reinterpret_cast<char*>(&WavHead), sizeof(OMC_WAV_header));

		Offset += sizeof(OMC_WAV_header) + WavHead.chunk_size;

		if (WavHead.chunk_size == 0)
		{
			SampleID++; 
			continue;
		}

		vector<char> Buffer (WavHead.chunk_size);
		ifile->read(&Buffer[0], WavHead.chunk_size);

		omc_rearrange(&Buffer[0], WavHead.chunk_size);
		omc_xor(&Buffer[0], WavHead.chunk_size, acc_keybyte, acc_counter);

		int ifmt;

		switch (WavHead.bits_per_sample)
		{
		case 8:
			ifmt = SF_FORMAT_PCM_U8;
			break;
		case 16:
			ifmt = SF_FORMAT_PCM_16;
			break;
		case 24:
			ifmt = SF_FORMAT_PCM_24;
			break;
		case 32:
			ifmt = SF_FORMAT_PCM_32;
			break;
		default:
				ifmt = 0;
		}

		SF_INFO Info;
		Info.format = ifmt | SF_FORMAT_RAW;
		Info.samplerate = WavHead.sample_rate;
		Info.channels = WavHead.num_channels;

		SFM30 ToLoad;
		ToLoad.Buffer = move(Buffer);
		ToLoad.DataLength = WavHead.chunk_size;

		auto NewSample = make_shared<SoundSample>();
		TemporaryState.File = sf_open_virtual(&M30Interface, SFM_READ, &Info, &ToLoad);
		TemporaryState.Info = &Info;
		TemporaryState.Enabled = OJM_WAV;
		NewSample->SetPitch(Speed);
		NewSample->Open(this);
		TemporaryState.Enabled = false;

		Arr[SampleID] = NewSample;
		SampleID++;
	}

	SampleID = 1000; // We start from the first OGG file..

	while (Offset < Head.fsize)
	{
		CheckInterruption();

		OMC_OGG_header OggHead;
		ifile->read(reinterpret_cast<char*>(&OggHead), sizeof(OMC_OGG_header));

		Offset += sizeof(OMC_OGG_header) + OggHead.sample_size;

		if (OggHead.sample_size == 0)
		{
			SampleID++;
			continue;
		}

		vector<char> Buffer (OggHead.sample_size);

		ifile->read(&Buffer[0], OggHead.sample_size);

		auto NewSample = make_shared<SoundSample>();

		SFM30 ToLoad;
		ToLoad.Buffer = Buffer;
		ToLoad.DataLength = OggHead.sample_size;

		OggVorbis_File vf;
		ov_open_callbacks(&ToLoad, &vf, nullptr, 0, M30InterfaceOgg);
		TemporaryState.File = &vf;
		TemporaryState.Info = vf.vi;
		TemporaryState.Enabled = OJM_OGG;
		NewSample->SetPitch(Speed);
		NewSample->Open(this);
		TemporaryState.Enabled = false;

		ov_clear(&vf);

		Arr[SampleID] = NewSample;
		SampleID++;
	}
}

bool AudioSourceOJM::HasDataLeft()
{
	return TemporaryState.Enabled != 0;
}

void AudioSourceOJM::SetPitch(double speed)
{
	Speed = speed;
}

size_t AudioSourceOJM::GetLength()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		auto Info = static_cast<SF_INFO*>(TemporaryState.Info);
		return Info->frames;
	}
	if (TemporaryState.Enabled == OJM_OGG)
	{
		return ov_pcm_total(static_cast<OggVorbis_File*>(TemporaryState.File), -1);
	}
	
	return 0;
}

uint32 AudioSourceOJM::GetRate()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		auto Info = static_cast<SF_INFO*>(TemporaryState.Info);
		return Info->samplerate;
	}
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		auto vi = static_cast<vorbis_info*>(TemporaryState.Info);
		return vi->rate;
	}
	else
		return 0;
}

void AudioSourceOJM::Seek(float Time)
{
	// Unused. 
}

shared_ptr<SoundSample> AudioSourceOJM::GetFromIndex(int index)
{
	return Arr[index-1];
}

uint32 AudioSourceOJM::GetChannels()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		auto Info = static_cast<SF_INFO*>(TemporaryState.Info);
		return Info->channels;
	}
	
	if (TemporaryState.Enabled == OJM_OGG)
	{
		auto vi = static_cast<vorbis_info*>(TemporaryState.Info);
		return vi->channels;
	}
	
	return 0;
}


bool AudioSourceOJM::IsValid()
{
	return TemporaryState.Enabled != 0;
}

bool AudioSourceOJM::Open(const char* f)
{
	char sig[4];

	ifile = make_shared<std::ifstream>(f, std::ios::binary);

	if (!ifile->is_open())
	{
		Log::Printf("AudioSourceOJM: unable to load %s.\n", f);
		return false;
	}

	ifile->read(sig, 4);

	switch (GetContainerKind(sig))
	{
	case M30:
		parseM30();
		break;
	case OMC:
		parseOMC();
		break;
	default:
		return false;
	}

	ifile->close();
	return true;
}

uint32 AudioSourceOJM::Read(short* buffer, size_t count)
{
	vector<short> temp_buf(count);
	size_t read = 0;
	if (TemporaryState.Enabled == 0)
		return 0;

	if (TemporaryState.Enabled == OJM_WAV)
	{
		read = sf_read_short(static_cast<SNDFILE*>(TemporaryState.File), temp_buf.data(), count);

		CheckInterruption();
	}
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		auto size = count * sizeof(short);
		while (read < size)
		{
			int sect;
			int res = ov_read(static_cast<OggVorbis_File*>(TemporaryState.File), reinterpret_cast<char*>(temp_buf.data()) + read, size - read, 0, 2, 1, &sect);

			if (res > 0)
				read += res;
			if (res <= 0)
			{
				if (res < 0) Log::Printf("Error loading ogg (%d)\n", res);
				break;
			}

			CheckInterruption();
		}

		if (read < size)
			Log::Printf("AudioSourceOJM: PCM count differs from what's reported! (%d out of %d)\n", read, size);
	}

	return read; // We /KNOW/ we won't be overreading.
}
