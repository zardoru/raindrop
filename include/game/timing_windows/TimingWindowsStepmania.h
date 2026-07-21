#pragma once

#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsStepmania : public TimingWindows {
    public:
        void default_setup() override;
        void setup(double strictness, double scale) override;
    };
}
