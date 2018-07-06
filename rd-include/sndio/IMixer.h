#pragma once

class AudioStream;
class AudioSample;

class IMixer {
public:
    virtual void AddStream(AudioStream *Sound);
    virtual void RemoveStream(AudioStream* Sound);
    virtual void AddSample(AudioSample *Sound);
    virtual void RemoveSample(AudioSample* Sound);
    virtual void Update();
    virtual double GetLatency();
    virtual double GetRate();
    virtual double GetFactor();
    virtual double GetTime();
};