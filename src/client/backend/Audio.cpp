#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include <map>
#include <future>
#include <condition_variable>
#include "../structure/Configuration.h"
#include <portaudio/portaudio.h>

#include <sndio/Audiofile.h>

#include <TextAndFileUtil.h>

#define BUFF_SIZE 8192

#ifdef WIN32

#include <portaudio/pa_win_wasapi.h>
#include <portaudio/pa_win_ds.h>
#include <portaudio/pa_win_wdmks.h>
#include <atomic>
#include <pa_ringbuffer.h>
#include <cassert>

#endif

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

PaError OpenStream(PaStream **mStream, PaDeviceIndex Device, void *Sound, double &dLatency, PaStreamCallback Callback) {
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

    outputParams.hostApiSpecificStreamInfo = NULL;

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
                  Conversion::Widen(Pa_GetErrorText(Err)).c_str());
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

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData);

class PaMixer : public IMixer {
    PaStream *Stream;
    char *RingbufData{};
    PaUtilRingBuffer RingBuf{};

    double Latency{};
    double Rate;

    std::vector<AudioStream *> Streams;
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

    double GetRate() {
        return Rate;
    }

    void Initialize(bool StartThread) {
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
            OpenStream(&Stream, RequestedDevice - 1, (void *) this, Latency, Mix);

#ifdef WIN32
        if (UseWasapi && !Stream) {
            OpenStream(&Stream, GetWasapiDevice(), static_cast<void *>(this), Latency, Mix);
        }

        if (!Stream) {
            // This was a Wasapi problem. Retry without it.
            if (UseWasapi) {
                Log::Logf("AUDIO: Problem initializing WASAPI. Falling back to WDMKS.\n");
                UseWasapi = false;
            }

            OpenStream(&Stream, DefaultWDMKSDevice, static_cast<void *>(this), Latency, Mix);
            if (!Stream) {
                Log::Logf("AUDIO: Problem initializing WDMKS. Falling back to DirectSound.\n");
                OpenStream(&Stream, DefaultDSDevice, static_cast<void *>(this), Latency, Mix);

                if (!Stream) {
                    Log::Logf("AUDIO: Problem initializing DirectSound API. Falling back to default API.\n");
                    OpenStream(&Stream, Pa_GetDefaultOutputDevice(), static_cast<void *>(this), Latency, Mix);
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
                for (auto &item: Streams)
                    item->UpdateDecoder();
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

    void AddStream(AudioStream *Stream) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        Streams.push_back(Stream);
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void RemoveStream(AudioStream *Stream) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        for (auto i = Streams.begin(); i != Streams.end();) {
            if ((*i) == Stream) {
                i = Streams.erase(i);
                if (i != Streams.end())
                    continue;
                else
                    break;
            }

            ++i;
        }
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void AddSample(AudioSample *Sample) override {
        mutex_decoder.lock();
        mutex_stream.lock();
        Samples.push_back(Sample);
        mutex_stream.unlock();
        mutex_decoder.unlock();
    }

    void RemoveSample(AudioSample *Sample) override {
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

    double GetTime() override {
        return Pa_GetStreamTime(Stream);
    }

private:
    float ts[BUFF_SIZE * 2]{};
    float tsF[BUFF_SIZE * 2]{};

public:

    void WriteAndAdvanceStream(
            float *out,
            int samples,
            const PaStreamCallbackTimeInfo *timeInfo) {
        int count = samples;

        memset(out, 0, samples * sizeof(float));

        bool streaming = false;
        {
            mutex_stream.lock();
            for (auto &Stream: Streams) {
                /*
                 * first, update our clocks
                 * */
//                atomic_stream_time_t new_clock = Stream->dac_clock;
//                new_clock.clock_map_index = (new_clock.clock_map_index + 1) % 2;
//                auto& map = new_clock.clock_map[new_clock.clock_map_index];
//                Stream->dac_clock.store(new_clock);

                auto read_frames_start = Stream->GetReadFrames();
                auto read = Stream->Read(ts, samples);
                auto read_frames_end = Stream->GetReadFrames();

                if (read > 0) {
                    stream_time_map_t map{
                            timeInfo->outputBufferDacTime,
                            timeInfo->outputBufferDacTime + (read / 2) / GetRate(),
                            read_frames_start,
                            read_frames_end
                    };

                    Stream->QueueStreamClock(map);

                    for (size_t k = 0; k < read; k++)
                        out[k] += ts[k];
                }

                /**
                 * Copy read data into output
                 */

                streaming |= Stream->IsPlaying();
            }

            for (auto &Sample: Samples) {
                size_t read = Sample->Read(ts, samples);

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

    double GetLatency() const {
        return Latency;
    }

    double GetFactor() {
        return ConstFactor;
    }
};

int Mix(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) {
    auto *Mix = static_cast<PaMixer *>(userData);
    Mix->WriteAndAdvanceStream(static_cast<float *>(output), frameCount * 2, timeInfo);
    return 0;
}

/*************************/
/********** API **********/
/*************************/

void GetAudioInfo() {
    PaHostApiIndex ApiCount = Pa_GetHostApiCount();

    Log::Logf("AUDIO: The default API is %d\n", Pa_GetDefaultHostApi());

    for (PaHostApiIndex i = 0; i < ApiCount; i++) {
        const PaHostApiInfo *Index = Pa_GetHostApiInfo(i);
        Log::Logf("(%d) %s: Default Output: %d (Identifier: %d)\n", i, Index->name, Index->defaultOutputDevice,
                  Index->type);

#ifdef WIN32
        if (Index->type == paWASAPI)
            DefaultWasapiDevice = Index->defaultOutputDevice;
        else if (Index->type == paDirectSound)
            DefaultDSDevice = Index->defaultOutputDevice;
        else if (Index->type == paWDMKS)
            DefaultWDMKSDevice = Index->defaultOutputDevice;
#endif
    }

    Log::Logf("\nAUDIO: The audio devices are\n");

    PaDeviceIndex DevCount = Pa_GetDeviceCount();
    for (PaDeviceIndex i = 0; i < DevCount; i++) {
        const PaDeviceInfo *Info = Pa_GetDeviceInfo(i);
        if (Info->maxOutputChannels == 0) continue; // Skip input devices.
        Log::Logf("(%d): %s\n", i + 1, Info->name);
        Log::Logf("\thighLat: %f ms, lowLat: %f ma\n", Info->defaultHighOutputLatency * 1000,
                  Info->defaultLowOutputLatency * 1000);
        Log::Logf("\tsampleRate: %f, hostApi: %d\n", Info->defaultSampleRate, Info->hostApi);
        Log::Logf("\tmaxchannels: %d\n", Info->maxOutputChannels);
    }
}

void InitAudio() {
#ifndef NO_AUDIO
    PaError Err = Pa_Initialize();

    if (Err != 0) // Couldn't get audio, bail out
        return;

#ifdef WIN32
    ConfigurationVariable cfg_UseWasapi("UseWasapi", "Audio");
    UseWasapi = cfg_UseWasapi;
#endif

    UseThreadedDecoder = ConfigurationVariable("UseThreadedDecoder", "Audio");

    GetAudioInfo();

    PaMixer::GetInstance().Initialize(UseThreadedDecoder);
    assert(Err == 0);
#endif
}

double GetDeviceLatency() {
#ifndef NO_AUDIO
    return Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency;
#else
    return 0;
#endif
}


IMixer *GetMixer() {
    return &PaMixer::GetInstance();
}

void MixerUpdate() {
#ifndef NO_AUDIO
    if (!UseThreadedDecoder)
        PaMixer::GetInstance().Run();
#endif
}

double MixerGetLatency() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().GetLatency();
#else
    return 0;
#endif
}

double MixerGetRate() {
    return PaMixer::GetInstance().GetRate();
}

double MixerGetFactor() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().GetFactor();
#else
    return 0;
#endif
}

double MixerGetTime() {
#ifndef NO_AUDIO
    return PaMixer::GetInstance().GetTime();
#else
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).count() / 1000.0;
#endif
}
