#pragma once

class AudioStream;
class AudioSample;

class IMixer {
public:
    virtual void AddStream(AudioStream *Sound) = 0;
    virtual void RemoveStream(AudioStream* Sound) = 0;
    virtual void AddSample(AudioSample *Sound) = 0;
    virtual void RemoveSample(AudioSample* Sound) = 0;
    // virtual void Update() = 0;
    // virtual double GetLatency() = 0;
    virtual double GetRate() = 0;
    virtual double GetFactor() = 0;
    virtual double GetTime() = 0;
};