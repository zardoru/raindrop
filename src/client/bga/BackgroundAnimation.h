#pragma once

#include "Interruptible.h"
#include <ChartGroup.h>

class BackgroundAnimation : public Interruptible, public Drawable2D
{
public:
    explicit BackgroundAnimation(Interruptible* parent = nullptr);
    virtual ~BackgroundAnimation() = default;
    virtual void set_animation_time(double Time);
    virtual void load();
    virtual void finalize_loading();
    virtual void update(float Delta);

    virtual void on_hit();
    virtual void on_miss();
    void emit_draw_calls(DrawCallSink &sink) override;

    /* Can only be called from main thread if LoadNow = true! */
    static std::unique_ptr<BackgroundAnimation> create_bga_from_chart_group(
            uint8_t chart_index,
            const std::shared_ptr<otoworm::ChartGroup>& chart_group,
            Interruptible* context,
            bool load_now = false);
};

bool is_video_path(std::filesystem::path path);
