#pragma once

class AudioStream;
class AudioSample;

class IMixer {
public:
    virtual void add_stream(AudioStream *Sound) = 0;
    virtual void remove_stream(AudioStream* Sound) = 0;
    virtual void add_sample(AudioSample *Sound) = 0;
    virtual void remove_sample(AudioSample* Sound) = 0;
    // virtual void Update() = 0;
    // virtual double GetLatency() = 0;
    virtual double get_rate() = 0;
    virtual double get_factor() = 0;
    virtual double get_time() = 0;
};