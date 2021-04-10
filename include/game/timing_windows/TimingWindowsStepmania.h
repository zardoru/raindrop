#pragma once

#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsStepmania : public TimingWindows {
    public:
        void DefaultSetup() override;
        void Setup(double strictness, double scale) override;
    };
}
