#pragma once

#include <game/Song.h>

#include <ChartGroup.h>

namespace rd
{
    void ConvertFromOtoworm(const otoworm::ChartGroup& source, Song* out);
    void ConvertFromOtoworm(std::shared_ptr<otoworm::ChartGroup> source, Song* out);
}
