#pragma once

#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsO2Jam : public TimingWindows {
    public:
        void default_setup() override;
        void setup(double strictness, double scale) override;
    };


}