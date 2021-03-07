#pragma once

#include "Interruptible.h"

namespace rd
{
    class Song;
}

class BackgroundAnimation : public Interruptible, public Drawable2D
{
public:
    BackgroundAnimation(Interruptible* parent = nullptr);
    virtual ~BackgroundAnimation() = default;
    virtual void SetAnimationTime(double Time);
    virtual void Load();
    virtual void Validate();
    virtual void Update(float Delta);

    virtual void OnHit();
    virtual void OnMiss();
    virtual void Render();

    /* Can only be called from main thread if LoadNow = true! */
    static std::unique_ptr<BackgroundAnimation> CreateBGAFromSong(uint8_t DifficultyIndex, rd::Song& Input, Interruptible* context, bool LoadNow = false);
};

bool IsVideoPath(std::filesystem::path path);