#pragma once

#include <game/RaindropProcessedChart.h>

namespace rd {
    namespace NoteTransform
    {
        using RuntimeNoteLanes = std::array<RuntimeNoteStorage, otoworm::max_channels>;

        void Randomize(RuntimeNoteLanes &notes, int channel_count, bool respect_scratch, int seed);
        void Mirror(RuntimeNoteLanes &notes, int channel_count, bool respect_scratch = false);
        void MoveKeysoundsToBGM(unsigned char channels, RuntimeNoteLanes& notes_by_channel, std::vector<otoworm::AutoplaySound> &bg_ms, double drift);
        void TransformToBeats(unsigned char channels, RuntimeNoteLanes& notes_by_channel, const otoworm::TimingData &bps);
    }
}
