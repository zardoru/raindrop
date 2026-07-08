#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include <map>
#include <future>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <cstring>
#include <algorithm>

#include "../structure/Configuration.h"
#include <portaudio.h>

#include <sndio/Audiofile.h>

#include <text_and_file_util.h>

#define BUFF_SIZE 8192

#ifdef WIN32

#include <pa_win_wasapi.h>
#include <pa_win_ds.h>
#include <pa_win_wdmks.h>

#endif

#ifdef LINUX
#include <pa_linux_alsa.h>
#endif

#include <pa_ringbuffer.h>

#include "Audio.h"

#include "Logging.h"

bool UseThreadedDecoder = false;

#ifdef WIN32
bool UseWasapi = false;
PaDeviceIndex DefaultWasapiDevice;
PaDeviceIndex DefaultDSDevice;
PaDeviceIndex DefaultWDMKSDevice;
#endif

/*************************/
/********* Mixer *********/
/*************************/

PaError OpenStream(PaStream **mStream, const PaDeviceIndex Device, void *Sound, double &dLatency, const PaStreamCallback Callback) {
    PaStreamParameters outputParams;

    outputParams.device = Device;
    outputParams.channelCount = 2;
    outputParams.sampleFormat = paFloat32;

    CfgVar RequestedLatency("RequestedLatency", "Audio");
    CfgVar UseHighLatency("UseHighLatency", "Audio");

#ifdef WIN32
    bool useAuto = (RequestedLatency < 0 && UseWasapi) || (RequestedLatency <= 0 && !UseWasapi);
#else
    bool useAuto = RequestedLatency <= 0.0;
#endif
    if (useAuto || UseHighLatency) {
        if (!UseHighLatency)
            outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
        else
            outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultHighOutputLatency;
    } else {
        outputParams.suggestedLatency = RequestedLatency.flt() / 1000.f;
    }

    Log::Logf("AUDIO: Requesting latency of %f ms\n", outputParams.suggestedLatency * 1000);

#ifndef WIN32

    outputParams.hostApiSpecificStreamInfo = nullptr;

#else
    PaWasapiStreamInfo StreamInfo;
    PaWinDirectSoundStreamInfo DSStreamInfo;
    PaWinWDMKSInfo WDMKSStreamInfo;
    if (UseWasapi) {
        outputParams.hostApiSpecificStreamInfo = &StreamInfo;
        StreamInfo.hostApiType = paWASAPI;
        StreamInfo.size = sizeof(PaWasapiStreamInfo);
        StreamInfo.version = 1;

        CfgVar UseSharedMode("WasapiUseSharedMode", "Audio");
        if (!UseSharedMode) {
            Log::Logf("AUDIO: Attempting to use exclusive mode WASAPI\n");
            StreamInfo.threadPriority = eThreadPriorityProAudio;
            StreamInfo.flags = paWinWasapiExclusive | paWinWasapiThreadPriority;
        } else {
            Log::Logf("AUDIO: Attempting to use shared mode WASAPI\n");
            StreamInfo.threadPriority = eThreadPriorityGames;
            StreamInfo.flags = 0;
        }

        StreamInfo.hostProcessorOutput = nullptr;
        StreamInfo.hostProcessorInput = nullptr;
    } else {
        if (Pa_GetHostApiInfo(Pa_GetDeviceInfo(Device)->hostApi)->type == paWDMKS) {
            Log::Logf("AUDIO: Attempting to use Windows Driver Model Kernel Streaming\n");
            outputParams.hostApiSpecificStreamInfo = &WDMKSStreamInfo;

            WDMKSStreamInfo.size = sizeof(PaWinWDMKSInfo);
            WDMKSStreamInfo.hostApiType = paWDMKS;
            WDMKSStreamInfo.version = 1;
            WDMKSStreamInfo.flags = 0;
            WDMKSStreamInfo.noOfPackets = 0;
            WDMKSStreamInfo.channelMask = 0;
        }

        if (Pa_GetHostApiInfo(Pa_GetDeviceInfo(Device)->hostApi)->type == paDirectSound) {
            Log::Logf("AUDIO: Attempting to use DirectSound\n");
            outputParams.hostApiSpecificStreamInfo = &DSStreamInfo;

            DSStreamInfo.size = sizeof(PaWinDirectSoundStreamInfo);
            DSStreamInfo.hostApiType = paDirectSound;
            DSStreamInfo.version = 2;
            DSStreamInfo.flags = 0;
        } else {
            if (Pa_GetHostApiInfo(Pa_GetDeviceInfo(Device)->hostApi)->type == paMME) {
                Log::Logf("AUDIO: Attempting to use MME\n");
            } else {
                Log::Logf("AUDIO: Opening host API with identifier: %d\n",
                          Pa_GetHostApiInfo(Pa_GetDeviceInfo(Device)->hostApi)->type);
            }
            outputParams.hostApiSpecificStreamInfo = nullptr;
        }
    }
#endif

    dLatency = outputParams.suggestedLatency;

    double Rate = Pa_GetDeviceInfo(Device)->defaultSampleRate;
    Log::Logf("AUDIO: Device Selected %d (Rate: %f)\n", Device + 1, Rate);
    // fire up portaudio
    PaError Err = Pa_OpenStream(mStream, nullptr, &outputParams, Rate, 0, 0, Callback, static_cast<void *>(Sound));

    if (Err) {
        Log::Logf("Audio: Failed opening device, portaudio reports \"%ls\"\n",
                  otoworm::locale::widen(Pa_GetErrorText(Err)).c_str());
    }
#ifdef LINUX
    else
    {
        Log::Logf("Audio: Enabling real time scheduling\n");
        PaAlsa_EnableRealtimeScheduling(mStream, true);
    }
#endif

    return Err;
}

#ifdef WIN32

PaDeviceIndex GetWasapiDevice() {
    return DefaultWasapiDevice; // implement properly?
}

#else
PaDeviceIndex GetWasapiDevice()
{
    return Pa_GetDefaultOutputDevice();
}
#endif

int mix(const void *input, void *output, unsigned long frame_count, const PaStreamCallbackTimeInfo *time_info,
        PaStreamCallbackFlags statusFlags, void *user_data);

class PaMixer : public IMixer {
    PaStream *Stream;
    char *RingbufData{};
    PaUtilRingBuffer RingBuf{};

    double Latency{};
    double Rate;

    std::vector<AudioStream *> active_streams;
    std::vector<AudioSample *> Samples;
    double ConstFactor{};

    int SizeAvailable{};
    bool Threaded{};
    std::atomic<bool> WaitForRingbufferSpace;

    std::mutex mutex_stream, mutex_decoder, rbufmux;
    std::condition_variable ringbuffer_has_space;

    PaMixer() {
        Rate = 44100;
        Stream = nullptr;
    }

public:


    static PaMixer &GetInstance() {
        static auto *Mixer = new PaMixer;
        return *Mixer;
    }

    double get_rate() override {
        return Rate;
    }

    void Initialize(const bool StartThread) {
        RingbufData = new char[BUFF_SIZE * sizeof(float)];

        WaitForRingbufferSpace = false;

        PaUtil_InitializeRingBuffer(&RingBuf, sizeof(float), BUFF_SIZE, RingbufData);

        Threaded = StartThread;
        Stream = nullptr;

        if (StartThread) {
            std::thread(&PaMixer::Run, this).detach();
        }

        CfgVar RequestedDevice("RequestedDevice", "Audio");
        if (RequestedDevice > 0)
            OpenStream(&Stream, RequestedDevice - 1, (void *) this, Latency, mix);

#ifdef WIN32
        if (UseWasapi && !Stream) {
            OpenStream(&Stream, GetWasapiDevice(), static_cast<void *>(this), Latency, mix);
        }

        if (!Stream) {
            // This was a Wasapi problem. Retry without it.
            if (UseWasapi) {
                Log::Logf("AUDIO: Problem initializing WASAPI. Falling back to WDMKS.\n");
                UseWasapi = false;
            }

            OpenStream(&Stream, DefaultWDMKSDevice, static_cast<void *>(this), Latency, mix);
            if (!Stream) {
                Log::Logf("AUDIO: Problem initializing WDMKS. Falling back to DirectSound.\n");
                OpenStream(&Stream, DefaultDSDevice, static_cast<void *>(this), Latency, mix);

                if (!Stream) {
                    Log::Logf("AUDIO: Problem initializing DirectSound API. Falling back to default API.\n");
                    OpenStream(&Stream, Pa_GetDefaultOutputDevice(), static_cast<void *>(this), Latency, mix);
                }
            }
        }
#endif

        if (Stream) {
            Pa_StartStream(Stream);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Latency = Pa_GetStreamInfo(Stream)->outputLatency;
            Rate = Pa_GetStreamInfo(Stream)->sampleRate;
            Log::Logf("AUDIO: Latency after opening stream = %fms \n", Latency * 1000);
        }

        ConstFactor = 1.0;
    }

    void Run() {
        do {
            WaitForRingbufferSpace = true;

            {
                mutex_decoder.lock();
                for (auto &item: active_streams)
                    item->update_decoder();
                mutex_decoder.unlock();
            }

            if (Threaded) {
                std::unique_lock<std::mutex> lock(rbufmux);

                while (WaitForRingbufferSpace) {
                    ringbuffer_has_space.wait(lock);
                }
            }
        } while (Threaded);
    }

    void add_stream(AudioStream *stream) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        if (const auto s = std::ranges::find(active_streams, stream); s == active_streams.end())
            active_streams.push_back(stream);

        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void remove_stream(AudioStream *stream) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        for (auto i = active_streams.begin(); i != active_streams.end();) {
            if ((*i) == stream) {
                i = active_streams.erase(i);
                if (i != active_streams.end())
                    continue;
                else
                    break;
            }

            ++i;
        }
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void add_sample(AudioSample *Sample) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        Samples.push_back(Sample);
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void remove_sample(AudioSample *Sample) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        for (auto i = Samples.begin(); i != Samples.end();) {
            if ((*i) == Sample) {
                i = Samples.erase(i);

                if (i == Samples.end())
                    break;
                else continue;
            }

            ++i;
        }
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    double get_time() override {
        return Pa_GetStreamTime(Stream);
    }

private:
    float ts[BUFF_SIZE * 2]{};

public:

    void write_and_advance_stream(
            float *out,
            const int samples,
            const PaStreamCallbackTimeInfo *time_info) {
        memset(out, 0, samples * sizeof(float));

        bool streaming = false;
        {
            mutex_stream.lock();
            for (const auto &stream: active_streams) {
                /*
                 * first, update our clocks
                 * */
//                atomic_stream_time_t new_clock = Stream->dac_clock;
//                new_clock.clock_map_index = (new_clock.clock_map_index + 1) % 2;
//                auto& map = new_clock.clock_map[new_clock.clock_map_index];
//                Stream->dac_clock.store(new_clock);

                auto read_frames_start = stream->get_read_frames();
                auto read = stream->read(ts, samples);
                auto read_frames_end = stream->get_read_frames();

                if (read > 0) {
                    stream_time_map_t map{
                            time_info->outputBufferDacTime,
                            time_info->outputBufferDacTime + (read / 2) / get_rate(),
                            read_frames_start,
                            read_frames_end
                    };

                    stream->queue_stream_clock(map);

                    for (size_t k = 0; k < read; k++)
                        out[k] += ts[k];
                }

                /**
                 * Copy read data into output
                 */

                streaming |= stream->is_playing();
            }

            for (auto &Sample: Samples) {
                size_t read = Sample->read(ts, samples);

                for (size_t k = 0; k < read; k++)
                    out[k] += ts[k];
            }

            mutex_stream.unlock();
        }

        if (streaming) {
            WaitForRingbufferSpace = false;
            ringbuffer_has_space.notify_one();
        }
    }

    double get_latency() const {
        return Latency;
    }

    double get_factor() override {
        return ConstFactor;
    }
};

int mix(const void *input, void *output, const unsigned long frame_count, const PaStreamCallbackTimeInfo *time_info,
        PaStreamCallbackFlags statusFlags, void *user_data) {
    auto *mix = static_cast<PaMixer *>(user_data);
    mix->write_and_advance_stream(static_cast<float *>(output), frame_count * 2, time_info);
    return 0;
}

/*************************/
/********** API **********/
/*************************/

void get_audio_info() {
    const PaHostApiIndex api_count = Pa_GetHostApiCount();

    Log::Logf("AUDIO: The default API is %d\n", Pa_GetDefaultHostApi());

    for (PaHostApiIndex i = 0; i < api_count; i++) {
        const PaHostApiInfo *index = Pa_GetHostApiInfo(i);
        Log::Logf("(%d) %s: Default Output: %d (Identifier: %d)\n", i, index->name, index->defaultOutputDevice,
                  index->type);

#ifdef WIN32
        if (index->type == paWASAPI)
            DefaultWasapiDevice = index->defaultOutputDevice;
        else if (index->type == paDirectSound)
            DefaultDSDevice = index->defaultOutputDevice;
        else if (index->type == paWDMKS)
            DefaultWDMKSDevice = index->defaultOutputDevice;
#endif
    }

    Log::Logf("\nAUDIO: The audio devices are\n");

    const PaDeviceIndex dev_count = Pa_GetDeviceCount();
    for (PaDeviceIndex i = 0; i < dev_count; i++) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (info->maxOutputChannels == 0) continue; // Skip input devices.
        Log::Logf("(%d): %s\n", i + 1, info->name);
        Log::Logf("\thighLat: %f ms, lowLat: %f ma\n", info->defaultHighOutputLatency * 1000,
                  info->defaultLowOutputLatency * 1000);
        Log::Logf("\tsampleRate: %f, hostApi: %d\n", info->defaultSampleRate, info->hostApi);
        Log::Logf("\tmaxchannels: %d\n", info->maxOutputChannels);
    }
}

void init_audio() {
#ifndef NO_AUDIO
    const PaError err = Pa_Initialize();

    if (err != 0) // Couldn't get audio, bail out
        return;

#ifdef WIN32
    ConfigurationVariable cfg_UseWasapi("UseWasapi", "Audio");
    UseWasapi = cfg_UseWasapi;
#endif

    UseThreadedDecoder = ConfigurationVariable("UseThreadedDecoder", "Audio");

    get_audio_info();

    PaMixer::GetInstance().Initialize(UseThreadedDecoder);
    assert(err == 0);
#endif
}

double get_device_latency() {
#ifndef NO_AUDIO
    return Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
#else
    return 0;
#endif
}


IMixer *get_mixer() {
    return &PaMixer::GetInstance();
}

void update_mixer() {
#ifndef NO_AUDIO
    if (!UseThreadedDecoder)
        PaMixer::GetInstance().Run();
#endif
}

double mixer_get_latency() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().get_latency();
#else
    return 0;
#endif
}

double mixer_get_rate() {
    return PaMixer::GetInstance().get_rate();
}

double mixer_get_factor() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().get_factor();
#else
    return 0;
#endif
}

double mixer_get_time() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().get_time();
#else
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).count() / 1000.0;
#endif
}
