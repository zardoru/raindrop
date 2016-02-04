#pragma once

#include "Sprite.h"

class GameObject : public Sprite
{
private:
    friend class ScreenEdit;

    bool BeingHeld;

    int32_t heldKey;

    unsigned char AnimationStatus;

public:

    double startTime, endTime, beat, hold_duration;
    uint32_t Measure;
    double Fraction;
    float fadeout_time, fadein_time; // time to fadeout, and time to get a hit
    float waiting_time;

    GameObject();
    static void GlobalInit();
    void Initialize();

    Judgment Hit(double time, Vec2 mpos, bool KeyDown, bool Autoplay, int32_t Key);
    Judgment Run(double delta, double Time, bool Autoplay);
    void Animate(float delta, float songTime);
    void Assign(double Duration, uint32_t Measure, double MeasureFraction);

    double GetFraction() const;
    void SetFraction(double frac);
    bool IsHold();
    void Invalidate();
    bool ShouldRemove();
};

using GameObjectVector = std::vector<GameObject>;