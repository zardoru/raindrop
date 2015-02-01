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
	char* Buffer;

	SFM30()
	{
		DataLength = 0; Buffer = NULL; Offset = 0;
	}
};

sf_count_t getFileLenM30(void* p)
{
	SFM30 *state = (SFM30*)p;
	return state->DataLength;
}

sf_count_t seekM30(sf_count_t offs, int whence, void* p)
{
	SFM30 *state = (SFM30*)p;

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
	SFM30 *state = (SFM30*)p;
	size_t toRead = min((size_t)count, (size_t)(state->DataLength - state->Offset));

	if (state->Offset >= state->DataLength)
		return 0;
	else
	{
		memcpy(ptr, state->Buffer + state->Offset, toRead);
		state->Offset += toRead;
	}

	return toRead;
}

sf_count_t tellM30(void* p)
{
	SFM30 *state = (SFM30*)p;
	return min(state->Offset, state->DataLength);
}

size_t readM30OGG(void* ptr, size_t size, size_t nmemb, void* p)
{
	SFM30 *state = (SFM30*)p;
	int toRead = min((unsigned int)(size*nmemb), (unsigned int)(state->DataLength - state->Offset));

	if (state->Offset >= state->DataLength)
		return 0;
	else
	{
		memcpy(ptr, state->Buffer + state->Offset, toRead);
		state->Offset += toRead;
	}

	return toRead;
}

int seekM30OGG(void* p, ogg_int64_t offs, int whence)
{
	SFM30 *state = (SFM30*)p;

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
	SFM30 *state = (SFM30*)p;
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
	char *buf_encoded = new char[len];
	memcpy(buf_encoded, buf_io, len);

	for (int block = 0; block < 17; block++)
	{
		int block_start_encoded = block_size * block;	// Where is the start of the enconded block
		int block_start_plain = block_size * REARRANGE_TABLE[key];	// Where the final plain block will be
		memcpy(buf_io + block_start_plain, buf_encoded + block_start_encoded, block_size);

		key++;
	}

	delete[] buf_encoded;
}

void omc_xor(char* buf, size_t len, int &acc_keybyte, int &acc_counter)
{
	int tmp = 0;
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

AudioSourceOJM::AudioSourceOJM()
{
	TemporaryState.Enabled = false;
	for (int i = 0; i < 2000; i++)
		Arr[i] = NULL;
}

AudioSourceOJM::~AudioSourceOJM()
{
	for (int i = 0; i < 2000; i++)
	{
		if (Arr[i])
		{
			MixerRemoveSample(Arr[i]);
			delete Arr[i];
		}
	}
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
	char* Buffer;
	ifile->read((char*)&Head, sizeof(M30Header));

	Buffer = new char[Head.payload_size];
	sizeLeft = Head.payload_size;

	for (int i = 0; i < Head.sample_count; i++)
	{
		if (sizeLeft < 52)
			break; // wrong number of samples

		M30Entry Entry;
		ifile->read((char*)&Entry, sizeof(M30Entry));
		sizeLeft -= sizeof(M30Entry);

		sizeLeft -= Entry.sample_size;

		int OJMIndex = Entry.ref;
		if (Entry.codec_code == 0)
			OJMIndex += 1000;
		else if (Entry.codec_code != 5) continue; // Unknown sample id type.

		char* SampleData = new char[Entry.sample_size];
		ifile->read(SampleData, Entry.sample_size);
		
		if (Head.encryption_flag & 16)
			NamiXOR(SampleData, Entry.sample_size);
		else if (Head.encryption_flag & 32)
			F412XOR(SampleData, Entry.sample_size);

		// Sample data is done. Now the bits that are specific to raindrop..
		SoundSample* NewSample = new SoundSample;

		SFM30 ToLoad;
		ToLoad.Buffer = SampleData;
		ToLoad.DataLength = Entry.sample_size;
		
		OggVorbis_File vf;

		ov_open_callbacks(&ToLoad, &vf, NULL, 0, M30InterfaceOgg);
		TemporaryState.File = &vf;
		TemporaryState.Info = vf.vi;

		if (vf.vi)
		{
			TemporaryState.Enabled = OJM_OGG;
			NewSample->Open(this);
			TemporaryState.Enabled = 0;
		}
		delete[] SampleData;
		ov_clear(&vf);
		
		MixerAddSample(NewSample);
		Arr[OJMIndex] = NewSample;
	}

	delete[] Buffer;
}

void AudioSourceOJM::parseOMC()
{
	OMC_header Head;
	int acc_keybyte = 0xFF;
	int acc_counter = 0;
	int Offset = 20;
	int SampleID = 0;
	ifile->read((char*)&Head, sizeof(OMC_header));


	// Parse WAV data first
	while (Offset < Head.ogg_start)
	{
		OMC_WAV_header WavHead;
		ifile->read((char*)&WavHead, sizeof(OMC_WAV_header));

		Offset += sizeof(OMC_WAV_header) + WavHead.chunk_size;

		if (WavHead.chunk_size == 0)
		{
			SampleID++; 
			continue;
		}

		char *Buffer = new char[WavHead.chunk_size];
		ifile->read(Buffer, WavHead.chunk_size);

		omc_rearrange(Buffer, WavHead.chunk_size);
		omc_xor(Buffer, WavHead.chunk_size, acc_keybyte, acc_counter);

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
		ToLoad.Buffer = Buffer;
		ToLoad.DataLength = WavHead.chunk_size;

		SoundSample* NewSample = new SoundSample;
		TemporaryState.File = (SNDFILE*)sf_open_virtual(&M30Interface, SFM_READ, &Info, &ToLoad);
		TemporaryState.Info = &Info;
		TemporaryState.Enabled = OJM_WAV;
		NewSample->Open(this);
		TemporaryState.Enabled = false;

		delete[] Buffer;

		Arr[SampleID] = NewSample;
		MixerAddSample(NewSample);
		SampleID++;
	}

	SampleID = 1000; // We start from the first OGG file..

	while (Offset < Head.fsize)
	{
		OMC_OGG_header OggHead;
		ifile->read((char*)&OggHead, sizeof(OMC_OGG_header));

		Offset += sizeof(OMC_OGG_header) + OggHead.sample_size;

		if (OggHead.sample_size == 0)
		{
			SampleID++;
			continue;
		}

		char *Buffer = new char[OggHead.sample_size];

		ifile->read(Buffer, OggHead.sample_size);

		SoundSample* NewSample = new SoundSample;

		SFM30 ToLoad;
		ToLoad.Buffer = Buffer;
		ToLoad.DataLength = OggHead.sample_size;

		OggVorbis_File vf;
		ov_open_callbacks(&ToLoad, &vf, NULL, 0, M30InterfaceOgg);
		TemporaryState.File = &vf;
		TemporaryState.Info = vf.vi;
		TemporaryState.Enabled = OJM_OGG;
		NewSample->Open(this);
		TemporaryState.Enabled = false;

		ov_clear(&vf);

		delete[] Buffer;

		Arr[SampleID] = NewSample;
		MixerAddSample(NewSample);
		SampleID++;
	}
}

bool AudioSourceOJM::HasDataLeft()
{
	return TemporaryState.Enabled != 0;
}

size_t AudioSourceOJM::GetLength()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		SF_INFO *Info = (SF_INFO*)TemporaryState.Info;
		return Info->frames;
	}
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		return ov_pcm_total((OggVorbis_File*)TemporaryState.File, -1);
	}
	else
		return 0;
}

uint32 AudioSourceOJM::GetRate()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		SF_INFO *Info = (SF_INFO*)TemporaryState.Info;
		return Info->samplerate;
	}
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		vorbis_info *vi = (vorbis_info*)TemporaryState.Info;
		return vi->rate;
	}
	else
		return 0;
}

void AudioSourceOJM::Seek(float Time)
{
	return;
}

SoundSample* AudioSourceOJM::GetFromIndex(int index)
{
	return Arr[index-1];
}

uint32 AudioSourceOJM::GetChannels()
{
	if (TemporaryState.Enabled == OJM_WAV)
	{
		SF_INFO *Info = (SF_INFO*)TemporaryState.Info;
		return Info->channels;
	}
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		vorbis_info *vi = (vorbis_info*)TemporaryState.Info;
		return vi->channels;
	}
	else
		return 0;
}


bool AudioSourceOJM::IsValid()
{
	return TemporaryState.Enabled != 0;
}

bool AudioSourceOJM::Open(const char* f)
{
	char sig[4];

	ifile = new std::ifstream(f, std::ios::binary);

	if (!*ifile)
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
	size_t read = 0;
	if (TemporaryState.Enabled == 0)
		return 0;

	if (TemporaryState.Enabled == OJM_WAV)
		read = sf_read_short((SNDFILE*)TemporaryState.File, buffer, count);
	else if (TemporaryState.Enabled == OJM_OGG)
	{
		size_t size = count * sizeof(short);
		while (read < size)
		{
			int sect;
			int res = ov_read((OggVorbis_File*)TemporaryState.File, (char*)buffer + read, size - read, 0, 2, 1, &sect);

			if (res > 0)
				read += res;
			if (res == 0)
				break;
		}

		if (read < size)
		{
			Log::Printf("AudioSourceOJM: PCM count differs from what's reported! (%d out of %d)\n", read, size);
		}
	}

	return read; // We /KNOW/ we won't be overreading.
}
