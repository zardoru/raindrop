#pragma once

namespace rd {
    namespace NoteTransform
    {
        void Randomize(VectorTrackNote &Notes, int ChannelCount, bool RespectScratch, int Seed);
        void Mirror(VectorTrackNote &Notes, int ChannelCount, bool RespectScratch = false);
        void MoveKeysoundsToBGM(unsigned char channels, VectorTrackNote notes_by_channel, std::vector<AutoplaySound> &bg_ms, double drift);
        void TransformToBeats(unsigned char channels, VectorTrackNote notes_by_channel, const TimingData &BPS);
    }
}
