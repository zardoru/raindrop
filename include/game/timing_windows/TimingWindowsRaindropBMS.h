#pragma once

#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsRaindropBMS : public TimingWindows {
    public:
        void DefaultSetup() override;
        void Setup(double strictness, double scale) override;
    };
}