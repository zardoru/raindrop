#pragma once

#include "Interruptible.h"
#include <ChartGroup.h>

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
    void emit_draw_calls(DrawCallSink &sink) override;

    /* Can only be called from main thread if LoadNow = true! */
    static std::unique_ptr<BackgroundAnimation> create_bga_from_chart_group(
            uint8_t chart_index,
            const std::shared_ptr<otoworm::ChartGroup>& chart_group,
            Interruptible* context,
            bool load_now = false);
};

bool IsVideoPath(std::filesystem::path path);
