#include "av-io-common-pch.h"
#include "Audiofile.h"
#include "AudioSourceOJM.h"

#include <fstream>
#include <sndfile.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

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
    int32_t file_format_version;
    int32_t encryption_flag;
    int32_t sample_count;
    int32_t sample_offset;
    int32_t payload_size;
    int32_t padding;
};

struct M30Entry
{
    char  sample_name[32];
    int32_t sample_size;
    int16_t codec_code;
    int16_t codec_code2;
    int32_t music_flag;
    int16_t ref;
    int16_t unk_zero;
    int32_t pcm_samples;
};

struct OMC_header
{
    int32_t unk;
    int32_t wav_start;
    int32_t ogg_start;
    int32_t fsize;
};

struct OMC_WAV_header
{
    char sample_name[32];
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate;
    int32_t bit_rate;
    int16_t block_align;
    int16_t bits_per_sample;
    int32_t unk_data;
    int32_t chunk_size;
};

struct OMC_OGG_header
{
    char sample_name[32];
    int32_t sample_size;
};

struct SFM30
{
    size_t DataLength;
    size_t Offset;
    std::vector<char> Buffer;

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

sf_count_t seekM30(const sf_count_t offs, const int whence, void* p)
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

sf_count_t readM30(void* ptr, const sf_count_t count, void* p)
{
    auto state = static_cast<SFM30*>(p);
    auto toRead = std::min(size_t(count), size_t(state->DataLength - state->Offset));

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
    return std::min(state->Offset, state->DataLength);
}

size_t readM30OGG(void* ptr, const size_t size, const size_t nmemb, void* p)
{
    auto state = static_cast<SFM30*>(p);
    int toRead = std::min((unsigned int)size*nmemb, (unsigned int)state->DataLength - state->Offset);

    if (state->Offset >= state->DataLength)
        return 0;
    else
    {
        memcpy(ptr, &state->Buffer[0] + state->Offset, toRead);
        state->Offset += toRead;
    }

    return toRead;
}

int seekM30OGG(void* p, const ogg_int64_t offs, const int whence)
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
    nullptr,
    tellM30
};

ov_callbacks M30InterfaceOgg = {
    readM30OGG,
    seekM30OGG,
    nullptr,
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
        0x04, 0x00 };

// AZ: Let me conserve the comment from the original source code for ojmdumper.
/**
* fuck the person who invented this, FUCK YOU!... but with love =$
*/
void omc_rearrange(char* buf_io, const size_t len)
{
    int key = ((len % 17) << 4) + (len % 17);
    int block_size = len / 17;
    std::vector<char> buf_encoded(len);
    memcpy(&buf_encoded[0], buf_io, len);

    for (int block = 0; block < 17; block++)
    {
        int block_start_encoded = block_size * block;	// Where is the start of the enconded block
        int block_start_plain = block_size * REARRANGE_TABLE[key];	// Where the final plain block will be
        memcpy(buf_io + block_start_plain, (&buf_encoded[0]) + block_start_encoded, block_size);

        key++;
    }
}

void omc_xor(char* buf, const size_t len, int &acc_keybyte, int &acc_counter)
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

void NamiXOR(char* buffer, const size_t length)
{
    char NAMI[] = { 0x6E, 0x61, 0x6D, 0x69 };
    for (size_t i = 0; i + 3 < length; i += 4)
    {
        buffer[i] ^= NAMI[0];
        buffer[i + 1] ^= NAMI[1];
        buffer[i + 2] ^= NAMI[2];
        buffer[i + 3] ^= NAMI[3];
    }
}

void F412XOR(char* buffer, const size_t length)
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
    TemporaryState.enabled = false;
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

    std::vector<char> Buffer(Head.payload_size);
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

        std::vector<char> SampleData(Entry.sample_size);
        ifile->read(&SampleData[0], Entry.sample_size);

        if (Head.encryption_flag & 16)
            NamiXOR(&SampleData[0], Entry.sample_size);
        else if (Head.encryption_flag & 32)
            F412XOR(&SampleData[0], Entry.sample_size);

        // Sample data is done. Now the bits that are specific to raindrop..
        auto new_sample = std::make_shared<AudioSample>();

        SFM30 to_load;
        to_load.Buffer = std::move(SampleData);
        to_load.DataLength = Entry.sample_size;

        OggVorbis_File vf;

        ov_open_callbacks(&to_load, &vf, nullptr, 0, M30InterfaceOgg);
        TemporaryState.file = &vf;
        TemporaryState.info = vf.vi;

        if (vf.vi)
        {
            TemporaryState.enabled = OJM_OGG;
            new_sample->set_pitch(Speed);
            new_sample->open(this);
            TemporaryState.enabled = 0;
        }

        ov_clear(&vf);

        arr_[OJMIndex] = new_sample;
    }
}

void AudioSourceOJM::parse_omc()
{
    OMC_header head;
    int acc_keybyte = 0xFF;
    int acc_counter = 0;
    int Offset = 20;
    int SampleID = 0;
    ifile->read(reinterpret_cast<char*>(&head), sizeof(OMC_header));

    // Parse WAV data first
    while (Offset < head.ogg_start)
    {
        CheckInterruption();

        OMC_WAV_header wav_head;
        ifile->read(reinterpret_cast<char*>(&wav_head), sizeof(OMC_WAV_header));

        Offset += sizeof(OMC_WAV_header) + wav_head.chunk_size;

        if (wav_head.chunk_size == 0)
        {
            SampleID++;
            continue;
        }

        std::vector<char> Buffer(wav_head.chunk_size);
        ifile->read(&Buffer[0], wav_head.chunk_size);

        omc_rearrange(&Buffer[0], wav_head.chunk_size);
        omc_xor(&Buffer[0], wav_head.chunk_size, acc_keybyte, acc_counter);

        int ifmt;

        switch (wav_head.bits_per_sample)
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

        SF_INFO info;
        info.format = ifmt | SF_FORMAT_RAW;
        info.samplerate = wav_head.sample_rate;
        info.channels = wav_head.num_channels;

        SFM30 to_load;
        to_load.Buffer = std::move(Buffer);
        to_load.DataLength = wav_head.chunk_size;

        auto NewSample = std::make_shared<AudioSample>();
        TemporaryState.file = sf_open_virtual(&M30Interface, SFM_READ, &info, &to_load);
        TemporaryState.info = &info;
        TemporaryState.enabled = OJM_WAV;
        NewSample->set_pitch(Speed);
        NewSample->open(this);
        TemporaryState.enabled = false;

        arr_[SampleID] = NewSample;
        SampleID++;
    }

    SampleID = 1000; // We start from the first OGG file..

    while (Offset < head.fsize)
    {
        CheckInterruption();

        OMC_OGG_header ogg_head;
        ifile->read(reinterpret_cast<char*>(&ogg_head), sizeof(OMC_OGG_header));

        Offset += sizeof(OMC_OGG_header) + ogg_head.sample_size;

        if (ogg_head.sample_size == 0)
        {
            SampleID++;
            continue;
        }

        std::vector<char> buffer(ogg_head.sample_size);

        ifile->read(&buffer[0], ogg_head.sample_size);

        auto new_sample = std::make_shared<AudioSample>();

        SFM30 to_load;
        to_load.Buffer = buffer;
        to_load.DataLength = ogg_head.sample_size;

        OggVorbis_File vf;
        ov_open_callbacks(&to_load, &vf, nullptr, 0, M30InterfaceOgg);
        TemporaryState.file = &vf;
        TemporaryState.info = vf.vi;
        TemporaryState.enabled = OJM_OGG;
        new_sample->set_pitch(Speed);
        new_sample->open(this);
        TemporaryState.enabled = false;

        ov_clear(&vf);

        arr_[SampleID] = new_sample;
        SampleID++;
    }
}

bool AudioSourceOJM::has_data_left()
{
    return TemporaryState.enabled != 0;
}

void AudioSourceOJM::set_pitch(const double speed)
{
    Speed = speed;
}

size_t AudioSourceOJM::get_length()
{
    if (TemporaryState.enabled == OJM_WAV)
    {
        auto Info = static_cast<SF_INFO*>(TemporaryState.info);
        return Info->frames;
    }
    if (TemporaryState.enabled == OJM_OGG)
    {
        return ov_pcm_total(static_cast<OggVorbis_File*>(TemporaryState.file), -1);
    }

    return 0;
}

uint32_t AudioSourceOJM::get_rate()
{
    if (TemporaryState.enabled == OJM_WAV)
    {
        auto Info = static_cast<SF_INFO*>(TemporaryState.info);
        return Info->samplerate;
    }
    else if (TemporaryState.enabled == OJM_OGG)
    {
        auto vi = static_cast<vorbis_info*>(TemporaryState.info);
        return vi->rate;
    }
    else
        return 0;
}

void AudioSourceOJM::seek(float time)
{
    // Unused.
}

std::shared_ptr<AudioSample> AudioSourceOJM::get_from_index(const int index)
{
    return arr_[index - 1];
}

uint32_t AudioSourceOJM::get_channels()
{
    if (TemporaryState.enabled == OJM_WAV)
    {
        auto Info = static_cast<SF_INFO*>(TemporaryState.info);
        return Info->channels;
    }

    if (TemporaryState.enabled == OJM_OGG)
    {
        auto vi = static_cast<vorbis_info*>(TemporaryState.info);
        return vi->channels;
    }

    return 0;
}

bool AudioSourceOJM::is_valid()
{
    return TemporaryState.enabled != 0;
}

bool AudioSourceOJM::open(const std::filesystem::path f)
{
    char sig[4];

    ifile = std::make_shared<std::ifstream>(f.string(), std::ios::binary);

    if (!ifile->is_open())
    {
        //Log::Printf("AudioSourceOJM: unable to load %s.\n", f.c_str());
        return false;
    }

    ifile->read(sig, 4);

    switch (GetContainerKind(sig))
    {
    case M30:
        parseM30();
        break;
    case OMC:
        parse_omc();
        break;
    default:
        return false;
    }

    ifile->close();
    return true;
}

uint32_t AudioSourceOJM::read(short* buffer, const size_t count)
{
    std::vector<short> temp_buf(count);
    size_t read = 0;
    if (TemporaryState.enabled == 0)
        return 0;

    if (TemporaryState.enabled == OJM_WAV)
    {
        read = sf_read_short(static_cast<SNDFILE*>(TemporaryState.file), temp_buf.data(), count);

        CheckInterruption();
    }
    else if (TemporaryState.enabled == OJM_OGG)
    {
        auto size = count * sizeof(short);
        while (read < size)
        {
            int sect;
            const long res = ov_read(
                    static_cast<OggVorbis_File*>(TemporaryState.file),
                       reinterpret_cast<char*>(temp_buf.data()) + read,
                       size - read,
                       0,
                       2,
                       1,
                       &sect
           );

            if (res > 0)
                read += res;
            if (res <= 0)
            {
                // if (res < 0) Log::Printf("Error loading ogg (%d)\n", res);
                break;
            }

            CheckInterruption();
        }

        //if (read < size)
        //    Log::Printf("AudioSourceOJM: PCM count differs from what's reported! (%d out of %d)\n", read, size);
    }

    std::ranges::copy(temp_buf, buffer);
    return read; // We /KNOW/ we won't be overreading.
}
